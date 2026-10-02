#pragma once

#include <cstdint>
#include <string>

#include "../core/app_state.hpp"
#include "../core/input.hpp"

namespace enku {

std::uint16_t keyboardCharacterKeyCount(KeyboardMode mode);
std::uint16_t keyboardKeyCount(KeyboardMode mode);
std::string keyboardKeyLabel(
    const KeyboardState& state,
    std::uint16_t index
);

enum class KeyboardRuntimeResult : std::uint8_t {
    Ignored,
    Changed,
    Done,
    Closed,
};

class KeyboardRuntime {
public:
    explicit KeyboardRuntime(KeyboardState& state);

    void open(KeyboardMode mode = KeyboardMode::Latin);
    KeyboardRuntimeResult handle(
        LogicalAction action,
        std::string& text
    );

    std::string focusedLabel() const;
    std::uint16_t keyCount() const;

private:
    KeyboardState& state_;

    KeyboardRuntimeResult navigate(int direction);
    KeyboardRuntimeResult activate(std::string& text);

    std::string keyLabel(std::uint16_t index) const;
    bool isLetterKey(std::uint16_t index) const;
    std::uint16_t characterKeyCount() const;
};

} // namespace enku
