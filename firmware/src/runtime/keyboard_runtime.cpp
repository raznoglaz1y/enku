#include "enku/runtime/keyboard_runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace enku {

namespace {

constexpr std::array<const char*, 26> kLatin = {
    "a","b","c","d","e","f","g","h","i","j","k","l","m",
    "n","o","p","q","r","s","t","u","v","w","x","y","z",
};

constexpr std::array<const char*, 33> kCyrillic = {
    "а","б","в","г","д","е","ё","ж","з","и","й","к","л","м","н","о",
    "п","р","с","т","у","ф","х","ц","ч","ш","щ","ъ","ы","ь","э","ю","я",
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
    switch (state_.mode) {
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

std::uint16_t KeyboardRuntime::keyCount() const {
    return characterKeyCount() + kSpecialCount;
}

std::string KeyboardRuntime::keyLabel(
    std::uint16_t index
) const {
    const auto chars = characterKeyCount();

    if (index < chars) {
        switch (state_.mode) {
            case KeyboardMode::Latin:
                return kLatin[index];
            case KeyboardMode::Cyrillic:
                return kCyrillic[index];
            case KeyboardMode::Symbols:
                return kSymbols[index];
            default:
                return {};
        }
    }

    switch (index - chars) {
        case 0: return "SPACE";
        case 1: return "BACKSPACE";
        case 2:
            return state_.shift ==
                       KeyboardShiftState::CapsLock
                ? "CAPS"
                : "SHIFT";
        case 3:
            return state_.mode == KeyboardMode::Latin
                ? "CYR"
                : state_.mode == KeyboardMode::Cyrillic
                    ? "123"
                    : "ABC";
        case 4: return "DONE";
        default: return {};
    }
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
