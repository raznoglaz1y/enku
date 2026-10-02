#pragma once

#include <cstdint>

namespace enku {

enum class PhysicalControl : std::uint8_t {
    Up,
    Function,
    Down,
    Boot,
    Power,
};

enum class PressType : std::uint8_t {
    Press,
    Release,
    Click,
    LongPress,
    Repeat,
};

enum class LogicalAction : std::uint8_t {
    None,
    NavigatePrevious,
    NavigateNext,
    Confirm,
    Back,
    OpenReaderMenu,
    OpenQuickTypography,
    PagePrevious,
    PageNext,
    Sleep,
    Wake,
    PowerOff,
};

struct PhysicalInputEvent {
    PhysicalControl control{PhysicalControl::Function};
    PressType press{PressType::Click};
};

struct ActionEvent {
    LogicalAction action{LogicalAction::None};
};

} // namespace enku
