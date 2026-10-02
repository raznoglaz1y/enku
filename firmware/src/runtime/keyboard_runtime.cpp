#include "enku/runtime/keyboard_runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace enku {

namespace {

constexpr std::array<const char*, 26> kLatin = {
    "q","w","e","r","t","y","u","i","o","p",
    "a","s","d","f","g","h","j","k","l",
    "z","x","c","v","b","n","m",
};

constexpr std::array<const char*, 33> kCyrillic = {
    "й","ц","у","к","е","н","г","ш","щ","з","х","ъ",
    "ф","ы","в","а","п","р","о","л","д","ж","э",
    "я","ч","с","м","и","т","ь","б","ю","ё",
};

constexpr std::array<const char*, 20> kSymbols = {
    "0","1","2","3","4","5","6","7","8","9",
    ".",
    ",",
    "-",
    "_",
    ":",
    ";",
    "!",
    "?",
    "(",
    ")",
};

constexpr std::uint16_t kSpecialCount = 5;

std::string asciiUpper(std::string value) {
    if (value.size() == 1U) {
        value[0] = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(value[0])
            )
        );
    }
    return value;
}

} // namespace

std::uint16_t keyboardCharacterKeyCount(
    KeyboardMode mode
) {
    switch (mode) {
        case KeyboardMode::Latin:
            return static_cast<std::uint16_t>(kLatin.size());
        case KeyboardMode::Cyrillic:
            return static_cast<std::uint16_t>(kCyrillic.size());
        case KeyboardMode::Symbols:
            return static_cast<std::uint16_t>(kSymbols.size());
        default:
            return 0;
    }
}

std::uint16_t keyboardKeyCount(
    KeyboardMode mode
) {
    return keyboardCharacterKeyCount(mode) +
           kSpecialCount;
}

std::string keyboardKeyLabel(
    const KeyboardState& state,
    std::uint16_t index
) {
    const auto chars =
        keyboardCharacterKeyCount(state.mode);

    std::string value;

    if (index < chars) {
        switch (state.mode) {
            case KeyboardMode::Latin:
                value = kLatin[index];
                break;
            case KeyboardMode::Cyrillic:
                value = kCyrillic[index];
                break;
            case KeyboardMode::Symbols:
                value = kSymbols[index];
                break;
            default:
                return {};
        }

        if (state.mode == KeyboardMode::Latin &&
            state.shift != KeyboardShiftState::Lowercase) {
            return asciiUpper(value);
        }

        return value;
    }

    switch (index - chars) {
        case 0: return "SPACE";
        case 1: return "DELETE";
        case 2:
            return state.shift ==
                       KeyboardShiftState::CapsLock
                ? "CAPS"
                : "SHIFT";
        case 3:
            return state.mode == KeyboardMode::Latin
                ? "CYR"
                : state.mode == KeyboardMode::Cyrillic
                    ? "123"
                    : "ABC";
        case 4: return "DONE";
        default: return {};
    }
}

KeyboardRuntime::KeyboardRuntime(
    KeyboardState& state
)
    : state_(state) {}

void KeyboardRuntime::open(KeyboardMode mode) {
    state_.open = true;
    state_.mode = mode;
    state_.shift = KeyboardShiftState::Lowercase;
    state_.focus_index = 0;
    state_.focused_label = focusedLabel();
}

KeyboardRuntimeResult KeyboardRuntime::handle(
    LogicalAction action,
    std::string& text
) {
    if (!state_.open) {
        return KeyboardRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return navigate(-1);
        case LogicalAction::NavigateNext:
            return navigate(1);
        case LogicalAction::Confirm:
            return activate(text);
        case LogicalAction::Back:
            state_.open = false;
            state_.focused_label.clear();
            return KeyboardRuntimeResult::Closed;
        default:
            return KeyboardRuntimeResult::Ignored;
    }
}

KeyboardRuntimeResult KeyboardRuntime::navigate(
    int direction
) {
    const int count = static_cast<int>(keyCount());
    int next =
        static_cast<int>(state_.focus_index) +
        direction;

    if (next < 0) {
        next = count - 1;
    } else if (next >= count) {
        next = 0;
    }

    state_.focus_index =
        static_cast<std::uint16_t>(next);
    state_.focused_label = focusedLabel();

    return KeyboardRuntimeResult::Changed;
}

KeyboardRuntimeResult KeyboardRuntime::activate(
    std::string& text
) {
    const auto chars = characterKeyCount();
    const auto index = state_.focus_index;

    if (index < chars) {
        auto value = keyLabel(index);

        if (state_.mode == KeyboardMode::Latin &&
            state_.shift != KeyboardShiftState::Lowercase) {
            value = asciiUpper(value);
        }

        text += value;

        if (state_.shift ==
            KeyboardShiftState::NextUppercase) {
            state_.shift =
                KeyboardShiftState::Lowercase;
            state_.focused_label = focusedLabel();
        }

        return KeyboardRuntimeResult::Changed;
    }

    switch (index - chars) {
        case 0:
            text += " ";
            return KeyboardRuntimeResult::Changed;

        case 1:
            if (!text.empty()) {
                auto pos = text.size() - 1U;
                while (pos > 0U &&
                       (static_cast<unsigned char>(text[pos]) & 0xC0U) ==
                           0x80U) {
                    --pos;
                }
                text.erase(pos);
            }
            return KeyboardRuntimeResult::Changed;

        case 2:
            if (state_.shift ==
                KeyboardShiftState::Lowercase) {
                state_.shift =
                    KeyboardShiftState::NextUppercase;
            } else if (state_.shift ==
                       KeyboardShiftState::NextUppercase) {
                state_.shift =
                    KeyboardShiftState::CapsLock;
            } else {
                state_.shift =
                    KeyboardShiftState::Lowercase;
            }
            return KeyboardRuntimeResult::Changed;

        case 3:
            if (state_.mode == KeyboardMode::Latin) {
                state_.mode = KeyboardMode::Cyrillic;
            } else if (state_.mode == KeyboardMode::Cyrillic) {
                state_.mode = KeyboardMode::Symbols;
            } else {
                state_.mode = KeyboardMode::Latin;
            }
            state_.focus_index = 0;
            state_.focused_label = focusedLabel();
            return KeyboardRuntimeResult::Changed;

        case 4:
            state_.open = false;
            state_.focused_label.clear();
            return KeyboardRuntimeResult::Done;

        default:
            return KeyboardRuntimeResult::Ignored;
    }
}

std::uint16_t KeyboardRuntime::characterKeyCount() const {
    return keyboardCharacterKeyCount(state_.mode);
}

std::uint16_t KeyboardRuntime::keyCount() const {
    return keyboardKeyCount(state_.mode);
}

std::string KeyboardRuntime::keyLabel(
    std::uint16_t index
) const {
    return keyboardKeyLabel(state_, index);
}

bool KeyboardRuntime::isLetterKey(
    std::uint16_t index
) const {
    return index < characterKeyCount();
}

std::string KeyboardRuntime::focusedLabel() const {
    auto label = keyLabel(state_.focus_index);

    if (state_.mode == KeyboardMode::Latin &&
        state_.focus_index < characterKeyCount() &&
        state_.shift != KeyboardShiftState::Lowercase) {
        return asciiUpper(label);
    }

    return label;
}

} // namespace enku
