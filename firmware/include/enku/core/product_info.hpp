#pragma once

#include <string_view>

namespace enku {

inline constexpr std::string_view kProductName =
    "ENKU";

inline constexpr std::string_view kProductDescription =
    "Repairable local-first e-reader";

inline constexpr std::string_view kFirmwareVersion =
    "0.1.0-dev";

inline constexpr std::string_view kHardwareTarget =
    "Waveshare ESP32-S3-ePaper-3.97";

inline constexpr std::string_view kDisplayDescription =
    "800 x 480 e-paper";

inline constexpr std::string_view kInputDescription =
    "Physical controls";

} // namespace enku
