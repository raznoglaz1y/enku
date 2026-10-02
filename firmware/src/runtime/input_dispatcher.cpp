#include "enku/runtime/input_dispatcher.hpp"

namespace enku {

InputDispatcher::InputDispatcher(
    AppState& app_state,
    LibraryRuntimeController& library,
    ReaderRuntimeController& reader,
    SleepWakeCoordinator& sleep_wake,
    PowerOffCoordinator& power_off
)
    : app_state_(app_state),
      library_(library),
      reader_(reader),
      sleep_wake_(sleep_wake),
      power_off_(power_off) {}

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

    switch (*action) {
        case LogicalAction::NavigatePrevious: {
            const auto result =
                library_.handle(
                    LibraryFocusPreviousRequested{}
                );

            return result == LibraryRuntimeResult::Applied ||
                   result == LibraryRuntimeResult::Ignored
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::NavigateNext: {
            const auto result =
                library_.handle(
                    LibraryFocusNextRequested{}
                );

            return result == LibraryRuntimeResult::Applied ||
                   result == LibraryRuntimeResult::Ignored
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::Confirm: {
            if (app_state_.screen != Screen::Library) {
                return InputDispatchResult::Unhandled;
            }

            const auto result =
                library_.handle(
                    OpenFocusedBookRequested{}
                );

            return result == LibraryRuntimeResult::Applied
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

            return result == ReaderRuntimeResult::Applied ||
                   result == ReaderRuntimeResult::EndOfBook
                ? InputDispatchResult::Applied
                : InputDispatchResult::Failed;
        }

        case LogicalAction::Back: {
            if (app_state_.screen == Screen::Reading) {
                const auto result =
                    reader_.handle(
                        BackRequested{}
                    );

                return result ==
                    ReaderRuntimeResult::Applied
                    ? InputDispatchResult::Applied
                    : InputDispatchResult::Failed;
            }

            return InputDispatchResult::Unhandled;
        }

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

        case LogicalAction::OpenReaderMenu:
        case LogicalAction::OpenQuickTypography:
        case LogicalAction::Sleep:
            return InputDispatchResult::Unhandled;

        case LogicalAction::None:
        default:
            return InputDispatchResult::Ignored;
    }
}

} // namespace enku
