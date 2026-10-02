#include "enku/runtime/book_details_runtime.hpp"

#include <algorithm>

namespace enku {

BookDetailsRuntime::BookDetailsRuntime(
    AppState& app_state,
    LibraryService& library,
    LibraryRuntimeController& library_runtime,
    ReaderRuntimeController& reader,
    CborReaderCheckpointService& checkpoints,
    BookDeleteService& delete_service,
    BookDetailsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      library_(library),
      library_runtime_(library_runtime),
      reader_(reader),
      checkpoints_(checkpoints),
      delete_service_(delete_service),
      renderer_(renderer),
      refresh_(refresh) {}

BookDetailsRuntimeResult BookDetailsRuntime::handle(
    const OpenFocusedBookDetailsRequested&
) {
    return openFocused();
}

std::optional<BookRecord>
BookDetailsRuntime::currentBook() const {
    if (!app_state_.book_details.book_id.has_value()) {
        return std::nullopt;
    }

    return library_.get(
        *app_state_.book_details.book_id
    );
}

const char* BookDetailsRuntime::primaryActionLabel() const {
    const auto book = currentBook();

    if (!book.has_value()) {
        return "OPEN";
    }

    switch (book->reading_state) {
        case ReadingState::New:
            return "START";

        case ReadingState::Reading:
            return "CONTINUE";

        case ReadingState::Finished:
            return "READ AGAIN";
    }

    return "OPEN";
}

bool BookDetailsRuntime::hasRestartAction(
    const BookRecord& book
) const {
    return book.reading_state != ReadingState::New ||
           book.progress > 0.0F;
}

BookDetailsRuntimeResult
BookDetailsRuntime::openFocused() {
    if (app_state_.screen != Screen::Library ||
        !app_state_.library.focused_book.has_value()) {
        return BookDetailsRuntimeResult::Ignored;
    }

    const auto book =
        library_.get(
            *app_state_.library.focused_book
        );

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    app_state_.book_details =
        BookDetailsState{};
    app_state_.book_details.book_id =
        book->book_id;
    app_state_.book_details.focus =
        BookDetailsFocus::Primary;
    app_state_.screen = Screen::BookDetails;

    return render();
}

BookDetailsRuntimeResult
BookDetailsRuntime::close() {
    app_state_.book_details =
        BookDetailsState{};
    app_state_.screen = Screen::Library;

    const auto result =
        library_runtime_.redraw(
            RefreshReason::ScreenChanged,
            RefreshClass::Full
        );

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? BookDetailsRuntimeResult::Applied
        : BookDetailsRuntimeResult::Failed;
}

BookDetailsRuntimeResult
BookDetailsRuntime::render() {
    const auto book = currentBook();

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    if (renderer_ != nullptr &&
        !renderer_->renderBookDetails(
            app_state_,
            *book
        )) {
        return BookDetailsRuntimeResult::Failed;
    }

    if (refresh_ != nullptr) {
        RefreshRequest request;
        request.refresh_class = RefreshClass::Full;
        request.reason = RefreshReason::OverlayChanged;
        request.generation = ++refresh_generation_;
        request.may_coalesce = false;
        request.may_defer = false;

        if (!refresh_->submit(request)) {
            return BookDetailsRuntimeResult::Failed;
        }
    }

    return BookDetailsRuntimeResult::Applied;
}

BookDetailsRuntimeResult
BookDetailsRuntime::moveFocus(
    int direction
) {
    const auto book = currentBook();

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    if (app_state_.book_details.mode !=
        BookDetailsMode::Details) {
        if (app_state_.book_details.mode ==
            BookDetailsMode::RestartConfirm) {
            app_state_.book_details.confirm_restart =
                !app_state_.book_details.confirm_restart;
        } else if (
            app_state_.book_details.mode ==
            BookDetailsMode::DeleteConfirm) {
            app_state_.book_details.confirm_delete =
                !app_state_.book_details.confirm_delete;
        }

        return render();
    }

    int focus =
        static_cast<int>(
            app_state_.book_details.focus
        );

    constexpr int kFocusCount = 3;

    do {
        focus =
            (focus + direction + kFocusCount) %
            kFocusCount;

        const auto candidate =
            static_cast<BookDetailsFocus>(focus);

        if (candidate !=
                BookDetailsFocus::RestartReading ||
            hasRestartAction(*book)) {
            app_state_.book_details.focus =
                candidate;
            break;
        }
    } while (true);

    return render();
}

BookDetailsRuntimeResult
BookDetailsRuntime::activateFocused() {
    const auto book = currentBook();

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    if (app_state_.book_details.mode ==
        BookDetailsMode::RestartConfirm) {
        if (!app_state_.book_details.confirm_restart) {
            app_state_.book_details.mode =
                BookDetailsMode::Details;
            return render();
        }

        return restartReading();
    }

    if (app_state_.book_details.mode ==
        BookDetailsMode::DeleteConfirm) {
        if (!app_state_.book_details.confirm_delete) {
            app_state_.book_details.mode =
                BookDetailsMode::Details;
            return render();
        }

        return removeBook();
    }

    switch (app_state_.book_details.focus) {
        case BookDetailsFocus::Primary:
            if (book->reading_state ==
                ReadingState::Finished) {
                app_state_.book_details.mode =
                    BookDetailsMode::RestartConfirm;
                app_state_.book_details.confirm_restart =
                    false;
                return render();
            }

            app_state_.screen = Screen::Library;
            return reader_.handle(
                       OpenBookRequested{book->book_id}
                   ) == ReaderRuntimeResult::Applied
                ? BookDetailsRuntimeResult::Applied
                : BookDetailsRuntimeResult::Failed;

        case BookDetailsFocus::RestartReading:
            app_state_.book_details.mode =
                BookDetailsMode::RestartConfirm;
            app_state_.book_details.confirm_restart =
                false;
            return render();

        case BookDetailsFocus::DeleteBook:
            app_state_.book_details.mode =
                BookDetailsMode::DeleteConfirm;
            app_state_.book_details.confirm_delete =
                false;
            return render();
    }

    return BookDetailsRuntimeResult::Ignored;
}

BookDetailsRuntimeResult
BookDetailsRuntime::restartReading() {
    const auto book = currentBook();

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    const auto checkpoint_status =
        checkpoints_.erase(book->book_id);

    if (checkpoint_status != PersistStatus::Ok &&
        checkpoint_status != PersistStatus::NotFound) {
        return BookDetailsRuntimeResult::Failed;
    }

    if (library_.updateSummary(
            book->book_id,
            ReadingState::Reading,
            0.0F,
            book->last_opened_order
        ) != LibraryStatus::Ok) {
        return BookDetailsRuntimeResult::Failed;
    }

    app_state_.book_details =
        BookDetailsState{};
    app_state_.screen = Screen::Library;
    app_state_.library.focused_book =
        book->book_id;

    return reader_.handle(
               OpenBookRequested{book->book_id}
           ) == ReaderRuntimeResult::Applied
        ? BookDetailsRuntimeResult::Applied
        : BookDetailsRuntimeResult::Failed;
}

BookDetailsRuntimeResult
BookDetailsRuntime::removeBook() {
    const auto book = currentBook();

    if (!book.has_value()) {
        return BookDetailsRuntimeResult::Failed;
    }

    const auto removed =
        delete_service_.remove(book->book_id);

    if (removed != BookDeleteStatus::Ok) {
        return BookDetailsRuntimeResult::Failed;
    }

    app_state_.book_details =
        BookDetailsState{};
    app_state_.screen = Screen::Library;

    const auto result =
        library_runtime_.handle(
            LibraryRefreshRequested{}
        );

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? BookDetailsRuntimeResult::Applied
        : BookDetailsRuntimeResult::Failed;
}

BookDetailsRuntimeResult
BookDetailsRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::BookDetails) {
        return BookDetailsRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return moveFocus(-1);

        case LogicalAction::NavigateNext:
            return moveFocus(1);

        case LogicalAction::Confirm:
            return activateFocused();

        case LogicalAction::Back:
            if (app_state_.book_details.mode !=
                BookDetailsMode::Details) {
                app_state_.book_details.mode =
                    BookDetailsMode::Details;
                app_state_.book_details.confirm_delete =
                    false;
                app_state_.book_details.confirm_restart =
                    false;
                return render();
            }
            return close();

        default:
            return BookDetailsRuntimeResult::Ignored;
    }
}

} // namespace enku
