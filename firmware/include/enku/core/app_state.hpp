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

    bool search_origin_valid{false};
    std::uint32_t search_origin_offset{0};
    std::optional<BookId> search_origin_focused_book;

    std::uint32_t persistedOffset() const {
        return mode == LibraryQueryMode::Search &&
               search_origin_valid
            ? search_origin_offset
            : offset;
    }

    std::optional<BookId> persistedFocusedBook() const {
        return mode == LibraryQueryMode::Search &&
               search_origin_valid
            ? search_origin_focused_book
            : focused_book;
    }
};

enum class ContentsBookmarksTab : std::uint8_t {
    Contents,
    Bookmarks,
};

enum class ContentsBookmarksFocus : std::uint8_t {
    ContentsTab,
    BookmarksTab,
    Item,
};

struct ContentsBookmarksState {
    ContentsBookmarksTab tab{ContentsBookmarksTab::Contents};
    ContentsBookmarksFocus focus{ContentsBookmarksFocus::ContentsTab};
    std::uint32_t item_index{0};
    std::uint32_t window_start{0};
};

enum class BookDetailsMode : std::uint8_t {
    Details,
    DeleteConfirm,
    RestartConfirm,
};

enum class BookDetailsFocus : std::uint8_t {
    Primary,
    RestartReading,
    DeleteBook,
};

enum class BookFinishedFocus : std::uint8_t {
    BackToLibrary,
    ReadAgain,
};

struct BookFinishedState {
    BookFinishedFocus focus{BookFinishedFocus::BackToLibrary};
    bool restart_confirm{false};
    bool confirm_restart{false};
};

struct BookDetailsState {
    std::optional<BookId> book_id;
    BookDetailsMode mode{BookDetailsMode::Details};
    BookDetailsFocus focus{BookDetailsFocus::Primary};
    bool confirm_delete{false};
    bool confirm_restart{false};
};

enum class SettingsItem : std::uint8_t {
    Reading,
    Display,
    WiFi,
    Language,
    Storage,
    Sleep,
    About,
    PowerOff,
};

struct SettingsNavigationState {
    SettingsItem focus{SettingsItem::Reading};
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
    BookDetailsState book_details;
    BookFinishedState book_finished;
    ContentsBookmarksState contents_bookmarks;
    SettingsNavigationState settings_nav;
    KeyboardState keyboard;

    NetworkState network;
    WiFiPolicy wifi_policy{WiFiPolicy::AutoConnectTrusted};
    PowerState power;

    bool progress_dirty{false};
    bool import_active{false};
};

} // namespace enku
