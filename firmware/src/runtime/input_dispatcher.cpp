#include "enku/runtime/input_dispatcher.hpp"

namespace enku {

namespace {

InputDispatchResult dispatchOverlay(
    ReaderOverlayRuntime* overlay,
    SearchRuntime* search,
    ContentsBookmarksRuntime* contents_bookmarks,
    SleepWakeCoordinator& sleep_wake,
    LogicalAction action
) {
    if (overlay == nullptr) {
        return InputDispatchResult::Unhandled;
    }

    switch (overlay->handle(action)) {
        case ReaderOverlayRuntimeResult::Applied:
            return InputDispatchResult::Applied;

        case ReaderOverlayRuntimeResult::SearchRequested:
            if (search == nullptr) {
                return InputDispatchResult::Unhandled;
            }
            return search->openFromReader() ==
                SearchRuntimeResult::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;

        case ReaderOverlayRuntimeResult::ContentsBookmarksRequested:
            if (contents_bookmarks == nullptr) {
                return InputDispatchResult::Unhandled;
            }
            return contents_bookmarks->openFromReader() ==
                ContentsBookmarksRuntimeResult::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;

        case ReaderOverlayRuntimeResult::AboutBookRequested:
            return InputDispatchResult::Unhandled;

        case ReaderOverlayRuntimeResult::SleepRequested:
            return sleep_wake.sleep() ==
                SleepWakeStatus::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;

        case ReaderOverlayRuntimeResult::Ignored:
            return InputDispatchResult::Unhandled;

        case ReaderOverlayRuntimeResult::Failed:
        default:
            return InputDispatchResult::Failed;
    }
}

} // namespace

InputDispatcher::InputDispatcher(
    AppState& app_state,
    LibraryRuntimeController& library,
    ReaderRuntimeController& reader,
    SleepWakeCoordinator& sleep_wake,
    PowerOffCoordinator& power_off,
    ReaderOverlayRuntime* reader_overlay,
    SearchRuntime* search,
    LibrarySearchRuntime* library_search,
    BookDetailsRuntime* book_details,
    BookFinishedRuntime* book_finished,
    ContentsBookmarksRuntime* contents_bookmarks
)
    : app_state_(app_state),
      library_(library),
      reader_(reader),
      sleep_wake_(sleep_wake),
      power_off_(power_off),
      reader_overlay_(reader_overlay),
      search_(search),
      library_search_(library_search),
      book_details_(book_details),
      book_finished_(book_finished),
      contents_bookmarks_(contents_bookmarks) {}

InputDispatchResult InputDispatcher::handle(
    const PhysicalInputEvent& input
) {
    const auto action =
        InputActionMapper::map(
            app_state_,
            input
        );

    if (!action.has_value()) {
        return InputDispatchResult::Ignored;
    }

    if (app_state_.screen == Screen::ContentsBookmarks) {
        if (contents_bookmarks_ == nullptr) {
            return InputDispatchResult::Unhandled;
        }

        const auto result =
            contents_bookmarks_->handle(*action);

        if (result == ContentsBookmarksRuntimeResult::Applied) {
            return InputDispatchResult::Applied;
        }

        if (result == ContentsBookmarksRuntimeResult::Failed) {
            return InputDispatchResult::Failed;
        }

        return InputDispatchResult::Unhandled;
    }

    if (app_state_.screen == Screen::BookFinished) {
        if (book_finished_ == nullptr) {
            return InputDispatchResult::Unhandled;
        }

        const auto result =
            book_finished_->handle(*action);

        if (result == BookFinishedRuntimeResult::Applied) {
            return InputDispatchResult::Applied;
        }

        if (result == BookFinishedRuntimeResult::Failed) {
            return InputDispatchResult::Failed;
        }

        return InputDispatchResult::Unhandled;
    }

    if (app_state_.screen == Screen::BookDetails) {
        if (book_details_ == nullptr) {
            return InputDispatchResult::Unhandled;
        }

        const auto result =
            book_details_->handle(*action);

        if (result == BookDetailsRuntimeResult::Applied) {
            return InputDispatchResult::Applied;
        }

        if (result == BookDetailsRuntimeResult::Failed) {
            return InputDispatchResult::Failed;
        }

        return InputDispatchResult::Unhandled;
    }

    if (app_state_.screen == Screen::Search) {
        if (search_ == nullptr) {
            return InputDispatchResult::Unhandled;
        }

        const auto result = search_->handle(*action);
        if (result == SearchRuntimeResult::Applied) {
            return InputDispatchResult::Applied;
        }
        if (result == SearchRuntimeResult::Failed) {
            return InputDispatchResult::Failed;
        }
        return InputDispatchResult::Unhandled;
    }

    if (app_state_.screen == Screen::Library &&
        app_state_.library.mode == LibraryQueryMode::Search) {
        if (library_search_ == nullptr) {
            return InputDispatchResult::Unhandled;
        }

        const auto result =
            library_search_->handle(*action);

        if (result == LibrarySearchRuntimeResult::Applied) {
            return InputDispatchResult::Applied;
        }

        if (result == LibrarySearchRuntimeResult::Failed) {
            return InputDispatchResult::Failed;
        }

        // Once the keyboard is closed, result navigation falls
        // through to normal Library focus/open handling.
        if (app_state_.library.mode ==
                LibraryQueryMode::Search &&
            !app_state_.keyboard.open &&
            (*action == LogicalAction::NavigatePrevious ||
             *action == LogicalAction::NavigateNext ||
             *action == LogicalAction::Confirm)) {
            // Continue through the standard Library switch below.
        } else {
            return InputDispatchResult::Unhandled;
        }
    }

    switch (*action) {
        case LogicalAction::NavigatePrevious: {
            if (app_state_.screen ==
                Screen::ReaderOverlay) {
                return dispatchOverlay(
                    reader_overlay_,
                    search_,
                    contents_bookmarks_,
                    sleep_wake_,
                    *action
                );
            }

            const auto result =
                library_.handle(
                    LibraryFocusPreviousRequested{}
                );

            return result == LibraryRuntimeResult::Applied ||
                   result == LibraryRuntimeResult::Ignored ||
                   result == LibraryRuntimeResult::Empty
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::NavigateNext: {
            if (app_state_.screen ==
                Screen::ReaderOverlay) {
                return dispatchOverlay(
                    reader_overlay_,
                    search_,
                    contents_bookmarks_,
                    sleep_wake_,
                    *action
                );
            }

            const auto result =
                library_.handle(
                    LibraryFocusNextRequested{}
                );

            return result == LibraryRuntimeResult::Applied ||
                   result == LibraryRuntimeResult::Ignored ||
                   result == LibraryRuntimeResult::Empty
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::Confirm: {
            if (app_state_.screen ==
                Screen::ReaderOverlay) {
                return dispatchOverlay(
                    reader_overlay_,
                    search_,
                    contents_bookmarks_,
                    sleep_wake_,
                    *action
                );
            }

            if (app_state_.screen != Screen::Library) {
                return InputDispatchResult::Unhandled;
            }

            const auto result =
                library_.handle(
                    OpenFocusedBookRequested{}
                );

            return result == LibraryRuntimeResult::Applied ||
                   result == LibraryRuntimeResult::Ignored ||
                   result == LibraryRuntimeResult::Empty
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::PagePrevious: {
            const auto result =
                reader_.handle(
                    PagePreviousRequested{}
                );

            return result == ReaderRuntimeResult::Applied ||
                   result == ReaderRuntimeResult::BeginningOfBook
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::PageNext: {
            const auto result =
                reader_.handle(
                    PageNextRequested{}
                );

            if (result == ReaderRuntimeResult::EndOfBook) {
                if (book_finished_ == nullptr) {
                    return InputDispatchResult::Applied;
                }

                return book_finished_->openFromReader() ==
                    BookFinishedRuntimeResult::Applied
                    ? InputDispatchResult::Applied
                    : InputDispatchResult::Failed;
            }

            return result == ReaderRuntimeResult::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::Back: {
            if (app_state_.screen == Screen::Search) {
                if (search_ == nullptr) {
                    return InputDispatchResult::Unhandled;
                }

                return search_->handle(*action) ==
                    SearchRuntimeResult::Applied
                    ? InputDispatchResult::Applied
                    : InputDispatchResult::Failed;
            }

            if (app_state_.screen ==
                Screen::ReaderOverlay) {
                return dispatchOverlay(
                    reader_overlay_,
                    search_,
                    contents_bookmarks_,
                    sleep_wake_,
                    *action
                );
            }

            if (app_state_.screen == Screen::Reading) {
                const auto result =
                    reader_.handle(
                        BackRequested{}
                    );

                if (result != ReaderRuntimeResult::Applied) {
                    return InputDispatchResult::Failed;
                }

                const auto library_result =
                    library_.handle(
                        LibraryRefreshRequested{}
                    );

                return library_result ==
                           LibraryRuntimeResult::Applied ||
                       library_result ==
                           LibraryRuntimeResult::Empty
                    ? InputDispatchResult::Applied
                    : InputDispatchResult::Failed;
            }

            return InputDispatchResult::Unhandled;
        }

        case LogicalAction::OpenReaderMenu:
        case LogicalAction::OpenQuickTypography:
            return dispatchOverlay(
                reader_overlay_,
                search_,
                contents_bookmarks_,
                sleep_wake_,
                *action
            );

        case LogicalAction::Wake: {
            return sleep_wake_.wake() ==
                SleepWakeStatus::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::PowerOff: {
            return power_off_.powerOff() ==
                PowerOffStatus::Applied
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::Sleep:
            return InputDispatchResult::Unhandled;

        case LogicalAction::None:
        default:
            return InputDispatchResult::Ignored;
    }
}

} // namespace enku
