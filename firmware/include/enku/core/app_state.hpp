#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "types.hpp"
#include "localization.hpp"
#include "boot.hpp"

namespace enku {

struct TypographyState {
    std::string preset{"Standard"};
    std::uint16_t font_size_px{0};
    float line_spacing{1.0F};
    std::uint16_t margin_px{0};
    bool per_book_override{false};
};

struct LibraryState {
    LibraryView view{LibraryView::Grid};
    std::string filter;
    std::string sort;
    std::optional<BookId> focused_book;
};

struct NetworkState {
    bool connected{false};
    std::string ssid;
};

struct PowerState {
    std::uint8_t battery_percent{0};
    bool charging{false};
};

struct AppState {
    Screen screen{Screen::Boot};
    BootState boot;
    Orientation orientation{Orientation::Portrait};
    LocaleId ui_locale{LocaleId::En};

    LibraryState library;

    std::optional<BookId> current_book;
    std::optional<SemanticPosition> reading_position;
    TypographyState typography;

    NetworkState network;
    PowerState power;

    bool progress_dirty{false};
    bool import_active{false};
};

} // namespace enku
