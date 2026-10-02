#include "enku/runtime/input_runtime.hpp"

namespace enku {

std::optional<LogicalAction> InputActionMapper::map(
    const AppState& app_state,
    const PhysicalInputEvent& input
) {
    if (input.press != PressType::Click &&
        input.press != PressType::LongPress &&
        input.press != PressType::Repeat) {
        return std::nullopt;
    }

    if (app_state.screen == Screen::Sleep) {
        if (input.press == PressType::Click) {
            return LogicalAction::Wake;
        }
        return std::nullopt;
    }

    if (input.control == PhysicalControl::Power) {
        if (input.press == PressType::LongPress) {
            return LogicalAction::PowerOff;
        }
        return std::nullopt;
    }

    if (app_state.screen == Screen::Reading) {
        if (input.control == PhysicalControl::Up &&
            input.press == PressType::Click) {
            return LogicalAction::PagePrevious;
        }

        if (input.control == PhysicalControl::Down &&
            input.press == PressType::Click) {
            return LogicalAction::PageNext;
        }

        if (input.control == PhysicalControl::Function) {
            if (input.press == PressType::Click) {
                return LogicalAction::OpenReaderMenu;
            }

            if (input.press == PressType::LongPress) {
                return LogicalAction::OpenQuickTypography;
            }
        }

        if (input.control == PhysicalControl::Boot &&
            input.press == PressType::Click) {
            return LogicalAction::Back;
        }

        // Held page-turn buttons must never enqueue repeat page turns.
        return std::nullopt;
    }

    if (input.control == PhysicalControl::Up) {
        if (input.press == PressType::Click ||
            input.press == PressType::Repeat) {
            return LogicalAction::NavigatePrevious;
        }
        return std::nullopt;
    }

    if (input.control == PhysicalControl::Down) {
        if (input.press == PressType::Click ||
            input.press == PressType::Repeat) {
            return LogicalAction::NavigateNext;
        }
        return std::nullopt;
    }

    if (input.control == PhysicalControl::Function &&
        input.press == PressType::Click) {
        return LogicalAction::Confirm;
    }

    if (input.control == PhysicalControl::Boot &&
        input.press == PressType::Click) {
        return LogicalAction::Back;
    }

    return std::nullopt;
}

} // namespace enku
