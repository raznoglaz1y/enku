#include <cassert>
#include <string>

#include "enku/runtime/keyboard_runtime.hpp"

using namespace enku;

int main() {
    KeyboardState state;
    KeyboardRuntime keyboard(state);
    std::string text;

    keyboard.open(KeyboardMode::Latin);
    assert(state.open);
    assert(keyboard.focusedLabel() == "q");

    state.focus_index = 28;
    assert(
        keyboard.handle(
            LogicalAction::Confirm,
            text
        ) == KeyboardRuntimeResult::Changed
    );
    assert(
        state.shift ==
        KeyboardShiftState::NextUppercase
    );

    state.focus_index = 0;
    assert(
        keyboard.handle(
            LogicalAction::Confirm,
            text
        ) == KeyboardRuntimeResult::Changed
    );
    assert(text == "Q");
    assert(
        state.shift ==
        KeyboardShiftState::Lowercase
    );

    text.clear();
    keyboard.open(KeyboardMode::Cyrillic);
    state.focus_index = 35;
    assert(
        keyboard.handle(
            LogicalAction::Confirm,
            text
        ) == KeyboardRuntimeResult::Changed
    );
    state.focus_index = 0;
    assert(
        keyboard.handle(
            LogicalAction::Confirm,
            text
        ) == KeyboardRuntimeResult::Changed
    );
    assert(text == "Й");

    text = "й";
    keyboard.open(KeyboardMode::Cyrillic);
    state.focus_index = 34;
    assert(
        keyboard.handle(
            LogicalAction::Confirm,
            text
        ) == KeyboardRuntimeResult::Changed
    );
    assert(text.empty());

    return 0;
}
