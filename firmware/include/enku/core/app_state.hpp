#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "types.hpp"
#include "library.hpp"
#include "localization.hpp"
#include "boot.hpp"
#include "settings.hpp"
#include "reader_overlay.hpp"

namespace enku {

struct TypographyState {
    ReadingPreset preset{ReadingPreset::Standard};
    std::uint16_t font_size_px{18};
    float line_spacing{1.35F};
    std::uint16_t margin_px{24};
    bool per_book_override{false};
};

struct LibraryState {
    LibraryView view{LibraryView::Grid};
    LibraryQueryMode mode{LibraryQueryMode::Browse};
    LibraryFilter filter{LibraryFilter::All};
    LibrarySort sort{LibrarySort::RecentlyOpened};
    SortDirection direction{SortDirection::Descending};
    std::string search_text;
    std::uint32_t offset{0};
    std::uint16_t limit{24};
    std::uint32_t total_matches{0};
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

enum class KeyboardMode : std::uint8_t {
    Latin,
    Cyrillic,
    Symbols,
};

enum class KeyboardShiftState : std::uint8_t {
    Lowercase,
    NextUppercase,
    CapsLock,
};

struct KeyboardState {
    bool open{false};
    KeyboardMode mode{KeyboardMode::Latin};
    KeyboardShiftState shift{KeyboardShiftState::Lowercase};
    std::uint16_t focus_index{0};
    std::string focused_label;
};

enum class SearchPhase : std::uint8_t {
    QueryEntry,
    Results,
    NoResults,
};

struct SearchMatch {
    SemanticPosition position;
    std::string section_label;
    std::string preview;
    std::uint32_t preview_match_start{0};
    std::uint32_t preview_match_length{0};
};

struct ReaderSearchState {
    SearchPhase phase{SearchPhase::QueryEntry};
    std::optional<SemanticPosition> origin_position;
    std::string query;
    std::vector<SearchMatch> matches;
    std::uint32_t total_matches{0};
    std::uint32_t focus_index{0};
    std::uint32_t window_start{0};
};

struct ReaderSearchHighlight {
    std::optional<SemanticPosition> position;
    std::string query;
};

struct AppState {
    Screen screen{Screen::Boot};
    BootState boot;
    Orientation orientation{Orientation::Portrait};
    LocaleId ui_locale{LocaleId::En};

    LibraryState library;

    std::optional<BookId> current_book;
    std::optional<SemanticPosition> reading_position;
    float reading_progress{0.0F};
    bool current_book_finished{false};
    TypographyState typography;
    ReaderOverlayState reader_overlay;
    ReaderSearchState search;
    ReaderSearchHighlight search_highlight;
    KeyboardState keyboard;

    NetworkState network;
    WiFiPolicy wifi_policy{WiFiPolicy::AutoConnectTrusted};
    PowerState power;

    bool progress_dirty{false};
    bool import_active{false};
};

} // namespace enku
