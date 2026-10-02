#include "enku/runtime/reader_runtime.hpp"

namespace enku {

ReaderRuntimeController::ReaderRuntimeController(
    AppState& app_state,
    ReaderBookLoader& loader,
    RefreshService& refresh,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    TypographySettings typography,
    Viewport viewport
)
    : app_state_(app_state),
      loader_(loader),
      refresh_(refresh),
      library_(library),
      checkpoint_(checkpoint),
      typography_(typography),
      viewport_(viewport) {}

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

    app_state_.screen = Screen::BookOpening;
    app_state_.current_book = event.book_id;
    app_state_.current_book_finished = false;

    const auto refresh_result =
        submitRefresh(RefreshReason::ScreenChanged);

    if (refresh_result == ReaderRuntimeResult::RefreshRejected) {
        return refresh_result;
    }

    std::optional<SemanticPosition> saved_position;
    if (app_state_.reading_position.has_value() &&
        app_state_.reading_position->book_id == event.book_id) {
        saved_position = app_state_.reading_position;
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
    auto* active_session = session();

    if (app_state_.screen != Screen::Reading ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    return applySessionResult(active_session->next());
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const PagePreviousRequested&
) {
    auto* active_session = session();

    if (app_state_.screen != Screen::Reading ||
        active_session == nullptr ||
        !active_session->isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    return applySessionResult(active_session->previous());
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const BackRequested&
) {
    if (app_state_.screen != Screen::Reading ||
        !app_state_.current_book.has_value()) {
        return ReaderRuntimeResult::Ignored;
    }

    const BookId book_id = *app_state_.current_book;
    const ReadingState reading_state =
        app_state_.current_book_finished
            ? ReadingState::Finished
            : ReadingState::Reading;

    if (app_state_.progress_dirty &&
        app_state_.reading_position.has_value()) {
        const auto persist_status = checkpoint_.checkpoint(
            book_id,
            *app_state_.reading_position,
            app_state_.reading_progress,
            reading_state
        );

        if (persist_status != PersistStatus::Ok) {
            return ReaderRuntimeResult::CheckpointFailed;
        }

        app_state_.progress_dirty = false;
    }

    if (updateLibrarySummary(
            reading_state,
            app_state_.reading_progress
        ) != LibraryStatus::Ok) {
        return ReaderRuntimeResult::LibraryUpdateFailed;
    }

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

            const auto refresh_result = submitRefresh(
                RefreshReason::StatusChanged,
                true,
                false
            );

            if (refresh_result == ReaderRuntimeResult::RefreshRejected) {
                return refresh_result;
            }

            return ReaderRuntimeResult::EndOfBook;
        }

        case ReaderSessionStatus::LayoutFailed:
            return ReaderRuntimeResult::LayoutFailed;

        case ReaderSessionStatus::Closed:
        default:
            return ReaderRuntimeResult::Ignored;
    }
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

    return submitRefresh(RefreshReason::PageTurn);
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
