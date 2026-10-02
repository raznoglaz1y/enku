#pragma once

#include <cstdint>

namespace enku {

enum class DevicePowerState : std::uint8_t {
    Active,
    DisplayIdle,
    Suspended,
    PoweredOff,
};

enum class WakeReason : std::uint8_t {
    Unknown,
    PowerKey,
    NavigationInput,
    Rtc,
    ExternalPower,
    ColdBoot,
};

struct BatteryState {
    std::uint8_t percent{0};
    bool charging{false};
    bool external_power{false};
};

} // namespace enku
