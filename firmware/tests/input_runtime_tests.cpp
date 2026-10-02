#include "enku/runtime/input_runtime.hpp"

#include <cassert>

using namespace enku;

int main() {
    AppState app;

    app.screen = Screen::Library;

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Up,
                PressType::Click,
            }
        ) == LogicalAction::NavigatePrevious
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Repeat,
            }
        ) == LogicalAction::NavigateNext
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::Click,
            }
        ) == LogicalAction::Confirm
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Boot,
                PressType::Click,
            }
        ) == LogicalAction::Back
    );

    app.screen = Screen::Reading;

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Up,
                PressType::Click,
            }
        ) == LogicalAction::PagePrevious
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Click,
            }
        ) == LogicalAction::PageNext
    );

    assert(
        !InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Repeat,
            }
        ).has_value()
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::Click,
            }
        ) == LogicalAction::OpenReaderMenu
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::LongPress,
            }
        ) == LogicalAction::OpenQuickTypography
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Boot,
                PressType::Click,
            }
        ) == LogicalAction::Back
    );

    app.screen = Screen::ReaderOverlay;

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Up,
                PressType::Click,
            }
        ) == LogicalAction::NavigatePrevious
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Click,
            }
        ) == LogicalAction::NavigateNext
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::Click,
            }
        ) == LogicalAction::Confirm
    );

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Boot,
                PressType::Click,
            }
        ) == LogicalAction::Back
    );

    app.screen = Screen::Sleep;

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::Click,
            }
        ) == LogicalAction::Wake
    );

    assert(
        !InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Function,
                PressType::LongPress,
            }
        ).has_value()
    );

    app.screen = Screen::Library;

    assert(
        InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Power,
                PressType::LongPress,
            }
        ) == LogicalAction::PowerOff
    );

    assert(
        !InputActionMapper::map(
            app,
            PhysicalInputEvent{
                PhysicalControl::Up,
                PressType::Press,
            }
        ).has_value()
    );

    return 0;
}
