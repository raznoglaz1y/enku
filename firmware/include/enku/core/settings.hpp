#pragma once

#include <cstdint>
#include <string>

#include "library.hpp"
#include "localization.hpp"
#include "types.hpp"

namespace enku {

enum class ReadingPreset : std::uint8_t {
    Spacious,
    Comfortable,
    Standard,
    Compact,
    Dense,
    Custom,
};

enum class WiFiPolicy : std::uint8_t {
    Off,
    Manual,
    AutoConnectTrusted,
};

struct WiFiNetworkInfo {
    std::string ssid;
    std::int32_t rssi{0};
    bool secured{false};
};

struct GlobalSettings {
    LocaleId locale{LocaleId::En};
    Orientation orientation{Orientation::Portrait};

    LibraryView library_view{LibraryView::Grid};
    LibraryFilter library_filter{LibraryFilter::All};
    LibrarySort library_sort{LibrarySort::RecentlyOpened};
    SortDirection library_direction{SortDirection::Descending};

    ReadingPreset reading_preset{ReadingPreset::Standard};
    std::uint16_t font_size_px{18};
    float line_spacing{1.35F};
    std::uint16_t margin_px{24};

    WiFiPolicy wifi_policy{WiFiPolicy::AutoConnectTrusted};
};

} // namespace enku
