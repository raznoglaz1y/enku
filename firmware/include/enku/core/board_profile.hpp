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

enum class BoardFeature : std::uint8_t {
    Psram,
    RemovableStorage,
    NativeUsb,
    HardPowerSwitch,
    BatteryMeasurement,
    Imu,
    HallSensor,
    DockDetect,
    Frontlight,
    DualChannelFrontlight,
    WirelessCharging,
    RtcWakeSources,
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

constexpr bool supportsFeature(
    const BoardCapabilities& capabilities,
    BoardFeature feature
) {
    switch (feature) {
        case BoardFeature::Psram:
            return capabilities.has_psram;
        case BoardFeature::RemovableStorage:
            return capabilities.has_removable_storage;
        case BoardFeature::NativeUsb:
            return capabilities.has_native_usb;
        case BoardFeature::HardPowerSwitch:
            return capabilities.has_hard_power_switch;
        case BoardFeature::BatteryMeasurement:
            return capabilities.has_battery_measurement;
        case BoardFeature::Imu:
            return capabilities.has_imu;
        case BoardFeature::HallSensor:
            return capabilities.has_hall_sensor;
        case BoardFeature::DockDetect:
            return capabilities.has_dock_detect;
        case BoardFeature::Frontlight:
            return capabilities.has_frontlight;
        case BoardFeature::DualChannelFrontlight:
            return capabilities.has_dual_channel_frontlight;
        case BoardFeature::WirelessCharging:
            return capabilities.has_wireless_charging;
        case BoardFeature::RtcWakeSources:
            return capabilities.supports_rtc_wake_sources;
    }

    return false;
}

constexpr bool isReaderBoardProfileValid(
    const BoardProfile& profile,
    std::uint16_t required_width,
    std::uint16_t required_height
) {
    const auto& capabilities = profile.capabilities;

    return capabilities.has_removable_storage &&
        capabilities.display_controller ==
            DisplayControllerFamily::Ssd1677 &&
        capabilities.display_width ==
            required_width &&
        capabilities.display_height ==
            required_height &&
        capabilities.reading_controls >= 3U;
}

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
