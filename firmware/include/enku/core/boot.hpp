#pragma once

#include <cstdint>

namespace enku {

enum class BootStage : std::uint8_t {
    Reset,
    Diagnostics,
    PlatformMinimum,
    Storage,
    Persistence,
    RecoveryDecision,
    Display,
    Input,
    Restore,
    FirstScreen,
    Stable,
    RecoveryMode,
};

enum class BootMode : std::uint8_t {
    Normal,
    FirstStart,
    Recovery,
};

struct BootState {
    BootStage stage{BootStage::Reset};
    BootMode mode{BootMode::Normal};
    bool boot_in_progress{true};
    std::uint8_t incomplete_boot_count{0};
};

} // namespace enku
