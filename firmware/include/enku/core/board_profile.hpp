#pragma once

#include <cstdint>
#include <string_view>

namespace enku {

enum class BoardProfileId : std::uint8_t {
    WaveshareEsp32S3Epaper397,
    EnkuR01Base,
    EnkuR01Pro,
    EnkuR01ProWireless,
};

enum class BoardProfileMaturity : std::uint8_t {
    ReferenceHardware,
    DesignTarget,
    ValidatedHardware,
};

enum class StorageBus : std::uint8_t {
    None,
    Spi,
    SdMmc4Bit,
};

enum class DisplayControllerFamily : std::uint8_t {
    Unknown,
    Ssd1677,
};

enum class ReaderOrientation : std::uint8_t {
    Portrait,
    Landscape,
    PortraitInverted,
    LandscapeInverted,
};

struct BoardCapabilities {
    bool has_psram{false};
    bool has_removable_storage{false};
    bool has_native_usb{false};
    bool has_hard_power_switch{false};
    bool has_battery_measurement{false};
    bool has_imu{false};
    bool has_hall_sensor{false};
    bool has_dock_detect{false};
    bool has_frontlight{false};
    bool has_dual_channel_frontlight{false};
    bool has_wireless_charging{false};
    bool supports_rtc_wake_sources{false};

    std::uint8_t reading_controls{0};
    StorageBus storage_bus{StorageBus::None};
    DisplayControllerFamily display_controller{
        DisplayControllerFamily::Unknown
    };

    std::uint16_t display_width{0};
    std::uint16_t display_height{0};

    bool portrait{true};
    bool landscape{false};
    bool inverted_portrait{false};
    bool inverted_landscape{false};
};

struct BoardProfile {
    BoardProfileId id{
        BoardProfileId::WaveshareEsp32S3Epaper397
    };
    BoardProfileMaturity maturity{
        BoardProfileMaturity::ReferenceHardware
    };
    std::string_view name;
    BoardCapabilities capabilities;
};

constexpr bool supportsOrientation(
    const BoardCapabilities& capabilities,
    ReaderOrientation orientation
) {
    switch (orientation) {
        case ReaderOrientation::Portrait:
            return capabilities.portrait;
        case ReaderOrientation::Landscape:
            return capabilities.landscape;
        case ReaderOrientation::PortraitInverted:
            return capabilities.inverted_portrait;
        case ReaderOrientation::LandscapeInverted:
            return capabilities.inverted_landscape;
    }

    return false;
}

} // namespace enku
