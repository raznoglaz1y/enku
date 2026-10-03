#include "enku/runtime/reader_runtime.hpp"
#include "enku/runtime/reader_state_flush.hpp"

namespace enku {

ReaderRuntimeController::ReaderRuntimeController(
    AppState& app_state,
    ReaderBookLoader& loader,
    RefreshService& refresh,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    AppContextService& context,
    TypographySettings typography,
    Viewport viewport,
    ReaderPageRenderer* page_renderer
)
    : app_state_(app_state),
      loader_(loader),
      refresh_(refresh),
      library_(library),
      checkpoint_(checkpoint),
      context_(context),
      typography_(typography),
      native_viewport_(viewport),
      viewport_(viewport),
      page_renderer_(page_renderer) {}

ReaderSession* ReaderRuntimeController::session() {
    return loader_.session();
}

const ReaderSession* ReaderRuntimeController::session() const {
    return loader_.session();
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const OpenBookRequested& event
) {
    if (app_state_.screen != Screen::Library) {
        return ReaderRuntimeResult::Ignored;
    }

    typography_.font_size_px =
        app_state_.typography.font_size_px;
    typography_.line_spacing =
        app_state_.typography.line_spacing;
    typography_.margin_px =
        app_state_.typography.margin_px;

    viewport_ = orientedViewport();

    app_state_.screen = Screen::BookOpening;
    app_state_.current_book = event.book_id;
    app_state_.current_book_finished = false;

    const auto refresh_result =
        submitRefresh(RefreshReason::ScreenChanged);

    if (refresh_result == ReaderRuntimeResult::RefreshRejected) {
        return refresh_result;
    }

    std::optional<SemanticPosition> saved_position;

    ReaderCheckpoint restored;
    const auto restore_status =
        checkpoint_.load(event.book_id, restored);

    if (restore_status == PersistStatus::Ok) {
        if (restored.position.book_id == event.book_id) {
            saved_position = restored.position;
        }
    } else if (restore_status != PersistStatus::NotFound) {
        loader_.close();
        app_state_.library.focused_book = event.book_id;
        app_state_.screen = Screen::Library;
        app_state_.current_book.reset();
        app_state_.reading_position.reset();
        app_state_.reading_progress = 0.0F;
        app_state_.current_book_finished = false;
        app_state_.progress_dirty = false;

        const auto recovery_refresh =
            submitRefresh(RefreshReason::ErrorRecovery);
        if (recovery_refresh == ReaderRuntimeResult::RefreshRejected) {
            return recovery_refresh;
        }

        return ReaderRuntimeResult::CheckpointLoadFailed;
    }

    const BookLoadRequest request{
        event.book_id,
        saved_position,
        typography_,
        viewport_,
    };

    const auto load_result = loader_.open(request);
    if (!load_result.ok()) {
        return handle(BookOpenFailed{event.book_id});
    }

    return handle(BookOpened{event.book_id});
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const BookOpened& event
) {
    const auto* active_session = session();

    if (app_state_.screen != Screen::BookOpening ||
        !app_state_.current_book.has_value() ||
        *app_state_.current_book != event.book_id ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    const auto& current = active_session->currentPage();
    if (!current.has_value() ||
        current->first_position.book_id != event.book_id) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.screen = Screen::Reading;
    app_state_.current_book_finished = false;

    const auto state_result = commitVisiblePage(false);
    if (state_result != ReaderRuntimeResult::Applied) {
        return state_result;
    }

    app_state_.progress_dirty = false;
    page_turns_since_checkpoint_ = 0U;

    if (context_.save(
            AppRestoreContext{
                Screen::Reading,
                event.book_id,
                app_state_.library.persistedOffset(),
                app_state_.library.persistedFocusedBook(),
            }
        ) != PersistStatus::Ok) {
        loader_.close();
        app_state_.library.focused_book = event.book_id;
        app_state_.screen = Screen::Library;
        app_state_.current_book.reset();
        app_state_.reading_position.reset();
        app_state_.reading_progress = 0.0F;
        app_state_.current_book_finished = false;
        app_state_.progress_dirty = false;

        const auto recovery_refresh =
            submitRefresh(RefreshReason::ErrorRecovery);
        if (recovery_refresh == ReaderRuntimeResult::RefreshRejected) {
            return recovery_refresh;
        }

        return ReaderRuntimeResult::ContextSaveFailed;
    }

    return ReaderRuntimeResult::Applied;
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const BookOpenFailed& event
) {
    if (app_state_.screen != Screen::BookOpening ||
        !app_state_.current_book.has_value() ||
        *app_state_.current_book != event.book_id) {
        return ReaderRuntimeResult::Ignored;
    }

    loader_.close();
    app_state_.library.focused_book = event.book_id;
    app_state_.screen = Screen::Library;
    app_state_.current_book.reset();
    app_state_.reading_position.reset();
    app_state_.reading_progress = 0.0F;
    app_state_.current_book_finished = false;
    app_state_.progress_dirty = false;

    const auto refresh_result =
        submitRefresh(RefreshReason::ErrorRecovery);

    if (refresh_result == ReaderRuntimeResult::RefreshRejected) {
        return refresh_result;
    }

    return ReaderRuntimeResult::BookOpenFailed;
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const PageNextRequested&
) {
    app_state_.search_highlight =
        ReaderSearchHighlight{};

    auto* active_session = session();

    if (app_state_.screen != Screen::Reading ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    const auto result =
        applySessionResult(active_session->next());

    if (result == ReaderRuntimeResult::Applied) {
        recordPageTurnForCheckpoint();
    }

    return result;
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const PagePreviousRequested&
) {
    app_state_.search_highlight =
        ReaderSearchHighlight{};

    auto* active_session = session();

    if (app_state_.screen != Screen::Reading ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    const auto result =
        applySessionResult(active_session->previous());

    if (result == ReaderRuntimeResult::Applied) {
        recordPageTurnForCheckpoint();
    }

    return result;
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const TypographyDefaultsChanged&
) {
    auto* active_session = session();

    if (app_state_.screen != Screen::Reading &&
        app_state_.screen != Screen::ReaderOverlay &&
        app_state_.screen != Screen::ReadingSettings) {
        return ReaderRuntimeResult::Ignored;
    }

    if (active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    typography_.font_size_px =
        app_state_.typography.font_size_px;
    typography_.line_spacing =
        app_state_.typography.line_spacing;
    typography_.margin_px =
        app_state_.typography.margin_px;

    const bool was_dirty =
        app_state_.progress_dirty;

    active_session->invalidateLayout(
        typography_,
        viewport_
    );

    if (active_session->status() !=
        ReaderSessionStatus::Ready) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    const auto& current =
        active_session->currentPage();

    if (!current.has_value()) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.current_book =
        current->first_position.book_id;
    app_state_.reading_position =
        current->first_position;
    app_state_.reading_progress =
        current->progress;
    app_state_.progress_dirty = was_dirty;

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderPage(
            *current,
            typography_,
            app_state_.orientation
        )) {
        return ReaderRuntimeResult::RenderFailed;
    }

    if (app_state_.screen == Screen::ReaderOverlay ||
        app_state_.screen == Screen::ReadingSettings) {
        return ReaderRuntimeResult::Applied;
    }

    return submitRefresh(
        RefreshReason::ScreenChanged
    );
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const OrientationChanged&
) {
    auto* active_session = session();

    if (app_state_.screen != Screen::Reading ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    const bool was_dirty =
        app_state_.progress_dirty;

    viewport_ = orientedViewport();

    active_session->invalidateLayout(
        typography_,
        viewport_
    );

    if (active_session->status() !=
        ReaderSessionStatus::Ready) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    const auto* current =
        &active_session->currentPage();

    if (current == nullptr ||
        !current->has_value()) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.current_book =
        (*current)->first_position.book_id;
    app_state_.reading_position =
        (*current)->first_position;
    app_state_.reading_progress =
        (*current)->progress;
    app_state_.progress_dirty = was_dirty;

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderPage(
            **current,
            typography_,
            app_state_.orientation
        )) {
        return ReaderRuntimeResult::RenderFailed;
    }

    return submitRefresh(
        RefreshReason::ScreenChanged
    );
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const BookPositionChanged& event
) {
    auto* active_session = session();

    if ((app_state_.screen != Screen::Reading &&
         app_state_.screen != Screen::Search) ||
        active_session == nullptr ||
        !active_session->isOpen() ||
        !app_state_.current_book.has_value() ||
        event.position.book_id != *app_state_.current_book) {
        return ReaderRuntimeResult::Ignored;
    }

    const auto status =
        active_session->open(
            LayoutRequest{
                event.position.book_id,
                event.position,
                typography_,
                viewport_,
            }
        );

    if (status != ReaderSessionStatus::Ready) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.screen = Screen::Reading;
    app_state_.current_book_finished = false;

    return commitVisiblePage(true);
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const RemovableStorageLost&
) {
    if (!app_state_.current_book.has_value() ||
        !app_state_.reading_position.has_value()) {
        return ReaderRuntimeResult::Ignored;
    }

    const BookId book_id =
        *app_state_.current_book;

    ReaderStateFlushCoordinator flush(
        app_state_,
        library_,
        checkpoint_,
        context_
    );

    ReaderRuntimeResult recovery_result =
        ReaderRuntimeResult::Applied;

    switch (flush.flush(
        ReaderStateFlushTarget::ReturnToLibrary
    )) {
        case ReaderStateFlushStatus::Applied:
            break;
        case ReaderStateFlushStatus::CheckpointFailed:
            recovery_result =
                ReaderRuntimeResult::CheckpointFailed;
            break;
        case ReaderStateFlushStatus::LibraryUpdateFailed:
            recovery_result =
                ReaderRuntimeResult::LibraryUpdateFailed;
            break;
        case ReaderStateFlushStatus::ContextSaveFailed:
            recovery_result =
                ReaderRuntimeResult::ContextSaveFailed;
            break;
    }

    // Storage loss is a fail-safe boundary: even when persistence fails,
    // never leave a live reader session backed by removed media.
    page_turns_since_checkpoint_ = 0U;
    loader_.close();
    app_state_.library.focused_book = book_id;
    app_state_.screen = Screen::Library;
    app_state_.current_book.reset();
    app_state_.reading_position.reset();
    app_state_.reading_progress = 0.0F;
    app_state_.current_book_finished = false;
    app_state_.progress_dirty = false;

    return recovery_result;
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const BackRequested&
) {
    if (app_state_.screen != Screen::Reading ||
        !app_state_.current_book.has_value()) {
        return ReaderRuntimeResult::Ignored;
    }

    const BookId book_id = *app_state_.current_book;

    ReaderStateFlushCoordinator flush(
        app_state_,
        library_,
        checkpoint_,
        context_
    );

    switch (flush.flush(
        ReaderStateFlushTarget::ReturnToLibrary
    )) {
        case ReaderStateFlushStatus::Applied:
            break;
        case ReaderStateFlushStatus::CheckpointFailed:
            return ReaderRuntimeResult::CheckpointFailed;
        case ReaderStateFlushStatus::LibraryUpdateFailed:
            return ReaderRuntimeResult::LibraryUpdateFailed;
        case ReaderStateFlushStatus::ContextSaveFailed:
            return ReaderRuntimeResult::ContextSaveFailed;
    }

    page_turns_since_checkpoint_ = 0U;
    loader_.close();
    app_state_.library.focused_book = book_id;
    app_state_.screen = Screen::Library;
    app_state_.current_book.reset();
    app_state_.reading_position.reset();
    app_state_.reading_progress = 0.0F;
    app_state_.current_book_finished = false;

    return submitRefresh(RefreshReason::ScreenChanged);
}

ReaderRuntimeResult ReaderRuntimeController::applySessionResult(
    ReaderSessionStatus status
) {
    switch (status) {
        case ReaderSessionStatus::Ready:
            app_state_.current_book_finished = false;
            return commitVisiblePage(true);

        case ReaderSessionStatus::BeginningOfBook:
            return ReaderRuntimeResult::BeginningOfBook;

        case ReaderSessionStatus::EndOfBook: {
            if (!app_state_.current_book.has_value()) {
                return ReaderRuntimeResult::Ignored;
            }

            app_state_.current_book_finished = true;
            app_state_.reading_progress = 1.0F;
            app_state_.progress_dirty = true;

            if (updateLibrarySummary(
                    ReadingState::Finished,
                    1.0F
                ) != LibraryStatus::Ok) {
                return ReaderRuntimeResult::LibraryUpdateFailed;
            }

            // BookFinishedRuntime owns the completion-screen refresh.
            // Avoid refreshing the final reading framebuffer here, otherwise
            // e-paper would perform two consecutive full refreshes.
            return ReaderRuntimeResult::EndOfBook;
        }

        case ReaderSessionStatus::LayoutFailed:
            return ReaderRuntimeResult::LayoutFailed;

        case ReaderSessionStatus::Closed:
        default:
            return ReaderRuntimeResult::Ignored;
    }
}

ReaderRuntimeResult ReaderRuntimeController::redrawCurrentPage(
    RefreshReason reason
) {
    const auto* active_session = session();

    if (active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    const auto& current =
        active_session->currentPage();

    if (!current.has_value()) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderPage(
            *current,
            typography_,
            app_state_.orientation
        )) {
        return ReaderRuntimeResult::RenderFailed;
    }

    return submitRefresh(reason);
}

ReaderRuntimeResult ReaderRuntimeController::commitVisiblePage(
    bool mark_progress_dirty
) {
    const auto* active_session = session();
    if (active_session == nullptr) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    const auto& current = active_session->currentPage();
    if (!current.has_value()) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.current_book = current->first_position.book_id;
    app_state_.reading_position = current->first_position;
    app_state_.reading_progress = current->progress;
    app_state_.progress_dirty = mark_progress_dirty;

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderPage(
            *current,
            typography_,
            app_state_.orientation
        )) {
        return ReaderRuntimeResult::RenderFailed;
    }

    return submitRefresh(RefreshReason::PageTurn);
}

void ReaderRuntimeController::recordPageTurnForCheckpoint() {
    if (page_turns_since_checkpoint_ <
        kPeriodicCheckpointPageTurns) {
        ++page_turns_since_checkpoint_;
    }

    if (page_turns_since_checkpoint_ <
        kPeriodicCheckpointPageTurns) {
        return;
    }

    ReaderStateFlushCoordinator flush(
        app_state_,
        library_,
        checkpoint_,
        context_
    );

    if (flush.checkpointProgress() ==
            ReaderStateFlushStatus::Applied &&
        !app_state_.progress_dirty) {
        page_turns_since_checkpoint_ = 0U;
    }
}

ReaderRuntimeResult ReaderRuntimeController::submitRefresh(
    RefreshReason reason,
    bool may_coalesce,
    bool may_defer
) {
    RefreshRequest refresh_request;
    refresh_request.refresh_class = RefreshClass::Full;
    refresh_request.reason = reason;
    refresh_request.generation = ++refresh_generation_;
    refresh_request.may_coalesce = may_coalesce;
    refresh_request.may_defer = may_defer;

    if (!refresh_.submit(refresh_request)) {
        return ReaderRuntimeResult::RefreshRejected;
    }

    return ReaderRuntimeResult::Applied;
}

Viewport ReaderRuntimeController::orientedViewport() const {
    const auto short_side =
        native_viewport_.width < native_viewport_.height
            ? native_viewport_.width
            : native_viewport_.height;

    const auto long_side =
        native_viewport_.width > native_viewport_.height
            ? native_viewport_.width
            : native_viewport_.height;

    return app_state_.orientation == Orientation::Portrait
        ? Viewport{short_side, long_side}
        : Viewport{long_side, short_side};
}

LibraryStatus ReaderRuntimeController::updateLibrarySummary(
    ReadingState reading_state,
    float progress
) {
    if (!app_state_.current_book.has_value()) {
        return LibraryStatus::NotFound;
    }

    const auto record = library_.get(*app_state_.current_book);
    if (!record.has_value()) {
        return LibraryStatus::NotFound;
    }

    return library_.updateSummary(
        record->book_id,
        reading_state,
        progress,
        record->last_opened_order
    );
}

std::uint32_t ReaderRuntimeController::refreshGeneration() const {
    return refresh_generation_;
}

} // namespace enku
