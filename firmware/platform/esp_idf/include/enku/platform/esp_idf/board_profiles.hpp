#pragma once

#include "enku/core/board_profile.hpp"

namespace enku::platform::esp_idf::board_profiles {
namespace detail {

constexpr BoardCapabilities waveshareCapabilities() {
    BoardCapabilities value;
    value.has_psram = true;
    value.has_removable_storage = true;
    value.has_native_usb = true;
    value.has_battery_measurement = true;
    value.reading_controls = 4;
    value.storage_bus = StorageBus::SdMmc4Bit;
    value.display_controller =
        DisplayControllerFamily::Ssd1677;
    value.display_width = 800;
    value.display_height = 480;
    value.landscape = true;
    value.inverted_portrait = true;
    value.inverted_landscape = true;
    return value;
}

constexpr BoardCapabilities enkuBaseCapabilities() {
    BoardCapabilities value;
    value.has_psram = true;
    value.has_removable_storage = true;
    value.has_native_usb = true;
    value.has_hard_power_switch = true;
    value.has_battery_measurement = true;
    value.has_imu = true;
    value.has_hall_sensor = true;
    value.has_dock_detect = true;
    value.supports_rtc_wake_sources = true;
    value.reading_controls = 4;
    value.storage_bus = StorageBus::Spi;
    value.display_controller =
        DisplayControllerFamily::Ssd1677;
    value.display_width = 800;
    value.display_height = 480;
    value.landscape = true;
    value.inverted_portrait = true;
    value.inverted_landscape = true;
    return value;
}

constexpr BoardCapabilities enkuProCapabilities(
    bool wireless
) {
    auto value = enkuBaseCapabilities();
    value.has_frontlight = true;
    value.has_dual_channel_frontlight = true;
    value.has_wireless_charging = wireless;
    return value;
}

} // namespace detail

inline constexpr BoardProfile
    kWaveshareEsp32S3Epaper397{
        BoardProfileId::WaveshareEsp32S3Epaper397,
        BoardProfileMaturity::ReferenceHardware,
        "Waveshare ESP32-S3 ePaper 3.97",
        detail::waveshareCapabilities(),
    };

inline constexpr BoardProfile kEnkuR01Base{
    BoardProfileId::EnkuR01Base,
    BoardProfileMaturity::DesignTarget,
    "ENKU R0.1 Base",
    detail::enkuBaseCapabilities(),
};

inline constexpr BoardProfile kEnkuR01Pro{
    BoardProfileId::EnkuR01Pro,
    BoardProfileMaturity::DesignTarget,
    "ENKU R0.1 Pro",
    detail::enkuProCapabilities(false),
};

inline constexpr BoardProfile kEnkuR01ProWireless{
    BoardProfileId::EnkuR01ProWireless,
    BoardProfileMaturity::DesignTarget,
    "ENKU R0.1 Pro Wireless",
    detail::enkuProCapabilities(true),
};

} // namespace enku::platform::esp_idf::board_profiles
