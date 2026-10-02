#pragma once

#include <cstdint>

#include "settings.hpp"

namespace enku {

enum class ReaderOverlayMode : std::uint8_t {
    Menu,
    QuickTypography,
};

enum class ReaderMenuItem : std::uint8_t {
    Typography,
    ContentsBookmarks,
    AddBookmark,
    Search,
    Orientation,
    AboutBook,
    Sleep,
};

struct ReaderOverlayState {
    ReaderOverlayMode mode{ReaderOverlayMode::Menu};
    std::uint8_t focus_index{0};
    ReadingPreset preview_preset{ReadingPreset::Standard};
};

struct ReadingPresetValues {
    std::uint16_t font_size_px{18};
    float line_spacing{1.35F};
    std::uint16_t margin_px{24};
};

ReadingPresetValues readingPresetValues(
    ReadingPreset preset
);

} // namespace enku
