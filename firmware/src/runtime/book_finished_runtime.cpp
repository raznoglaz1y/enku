#include "enku/runtime/book_finished_runtime.hpp"

namespace enku {

BookFinishedRuntime::BookFinishedRuntime(
    AppState& app_state,
    LibraryService& library,
    LibraryRuntimeController& library_runtime,
    ReaderRuntimeController& reader,
    CborReaderCheckpointService& checkpoints,
    BookFinishedRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      library_(library),
      library_runtime_(library_runtime),
      reader_(reader),
      checkpoints_(checkpoints),
      renderer_(renderer),
      refresh_(refresh) {}

BookFinishedRuntimeResult
BookFinishedRuntime::openFromReader() {
    if (app_state_.screen != Screen::Reading ||
        !app_state_.current_book.has_value() ||
        !app_state_.current_book_finished) {
        return BookFinishedRuntimeResult::Ignored;
    }

    app_state_.book_finished =
        BookFinishedState{};
    app_state_.screen = Screen::BookFinished;

    return render();
}

BookFinishedRuntimeResult
BookFinishedRuntime::render() {
    if (!app_state_.current_book.has_value()) {
        return BookFinishedRuntimeResult::Failed;
    }

    const auto book =
        library_.get(
            *app_state_.current_book
        );

    if (!book.has_value()) {
        return BookFinishedRuntimeResult::Failed;
    }

    if (renderer_ != nullptr &&
        !renderer_->renderBookFinished(
            app_state_,
            *book
        )) {
        return BookFinishedRuntimeResult::Failed;
    }

    if (refresh_ != nullptr) {
        RefreshRequest request;
        request.refresh_class = RefreshClass::Full;
        request.reason = RefreshReason::StatusChanged;
        request.generation = ++refresh_generation_;
        request.may_coalesce = false;
        request.may_defer = false;

        if (!refresh_->submit(request)) {
            return BookFinishedRuntimeResult::Failed;
        }
    }

    return BookFinishedRuntimeResult::Applied;
}

BookFinishedRuntimeResult
BookFinishedRuntime::returnToLastPage() {
    app_state_.book_finished =
        BookFinishedState{};
    app_state_.screen = Screen::Reading;

    const auto result =
        reader_.redrawCurrentPage(
            RefreshReason::ScreenChanged
        );

    return result == ReaderRuntimeResult::Applied
        ? BookFinishedRuntimeResult::Applied
        : BookFinishedRuntimeResult::Failed;
}

BookFinishedRuntimeResult
BookFinishedRuntime::backToLibrary() {
    app_state_.book_finished =
        BookFinishedState{};
    app_state_.screen = Screen::Reading;

    const auto reader_result =
        reader_.handle(
            BackRequested{}
        );

    if (reader_result != ReaderRuntimeResult::Applied) {
        return BookFinishedRuntimeResult::Failed;
    }

    const auto library_result =
        library_runtime_.handle(
            LibraryRefreshRequested{}
        );

    return library_result == LibraryRuntimeResult::Applied ||
           library_result == LibraryRuntimeResult::Empty
        ? BookFinishedRuntimeResult::Applied
        : BookFinishedRuntimeResult::Failed;
}

BookFinishedRuntimeResult
BookFinishedRuntime::restartReading() {
    if (!app_state_.current_book.has_value()) {
        return BookFinishedRuntimeResult::Failed;
    }

    const BookId book_id =
        *app_state_.current_book;

    app_state_.screen = Screen::Reading;

    if (reader_.handle(
            BackRequested{}
        ) != ReaderRuntimeResult::Applied) {
        return BookFinishedRuntimeResult::Failed;
    }

    if (checkpoints_.erase(book_id) !=
        PersistStatus::Ok) {
        return BookFinishedRuntimeResult::Failed;
    }

    const auto book = library_.get(book_id);
    if (!book.has_value()) {
        return BookFinishedRuntimeResult::Failed;
    }

    if (library_.updateSummary(
            book_id,
            ReadingState::Reading,
            0.0F,
            book->last_opened_order
        ) != LibraryStatus::Ok) {
        return BookFinishedRuntimeResult::Failed;
    }

    app_state_.library.focused_book =
        book_id;

    const auto open_result =
        reader_.handle(
            OpenBookRequested{book_id}
        );

    return open_result == ReaderRuntimeResult::Applied
        ? BookFinishedRuntimeResult::Applied
        : BookFinishedRuntimeResult::Failed;
}

BookFinishedRuntimeResult
BookFinishedRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::BookFinished) {
        return BookFinishedRuntimeResult::Ignored;
    }

    if (app_state_.book_finished.restart_confirm) {
        switch (action) {
            case LogicalAction::NavigatePrevious:
            case LogicalAction::NavigateNext:
                app_state_.book_finished.confirm_restart =
                    !app_state_.book_finished.confirm_restart;
                return render();

            case LogicalAction::Confirm:
                if (!app_state_.book_finished.confirm_restart) {
                    app_state_.book_finished.restart_confirm =
                        false;
                    return render();
                }
                return restartReading();

            case LogicalAction::Back:
                app_state_.book_finished.restart_confirm =
                    false;
                app_state_.book_finished.confirm_restart =
                    false;
                return render();

            default:
                return BookFinishedRuntimeResult::Ignored;
        }
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
        case LogicalAction::NavigateNext:
            app_state_.book_finished.focus =
                app_state_.book_finished.focus ==
                        BookFinishedFocus::BackToLibrary
                    ? BookFinishedFocus::ReadAgain
                    : BookFinishedFocus::BackToLibrary;
            return render();

        case LogicalAction::Confirm:
            if (app_state_.book_finished.focus ==
                BookFinishedFocus::BackToLibrary) {
                return backToLibrary();
            }

            app_state_.book_finished.restart_confirm =
                true;
            app_state_.book_finished.confirm_restart =
                false;
            return render();

        case LogicalAction::Back:
            return returnToLastPage();

        default:
            return BookFinishedRuntimeResult::Ignored;
    }
}

} // namespace enku
