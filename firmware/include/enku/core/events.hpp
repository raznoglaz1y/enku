#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "types.hpp"
#include "input.hpp"
#include "diagnostics.hpp"
#include "boot.hpp"
#include "refresh.hpp"
#include "library.hpp"
#include "localization.hpp"
#include "settings.hpp"

namespace enku {

struct InputReceived {
    PhysicalInputEvent input;
};

struct ActionRequested {
    LogicalAction action{LogicalAction::None};
};

struct OpenBookRequested {
    BookId book_id;
};

struct LibraryRefreshRequested {};
struct OpenLibrarySearchRequested {};

struct LibraryFilterChanged {
    LibraryFilter filter{LibraryFilter::All};
};

struct LibrarySortChanged {
    LibrarySort sort{LibrarySort::RecentlyOpened};
    SortDirection direction{SortDirection::Descending};
};

struct LibraryViewChanged {
    LibraryView view{LibraryView::Grid};
};

struct LocaleChanged {
    LocaleId locale{LocaleId::En};
};

struct TypographyDefaultsChanged {
    ReadingPreset preset{ReadingPreset::Standard};
    std::uint16_t font_size_px{18};
    float line_spacing{1.35F};
    std::uint16_t margin_px{24};
};

struct WiFiPolicyChanged {
    WiFiPolicy policy{WiFiPolicy::AutoConnectTrusted};
};

struct LibrarySearchChanged {
    std::string text;
};

struct LibraryFocusNextRequested {};
struct LibraryFocusPreviousRequested {};
struct OpenFocusedBookRequested {};
struct OpenFocusedBookDetailsRequested {};
struct RestartFocusedBookRequested {};
struct RemoveFocusedBookRequested {};
struct DeleteFocusedBookRequested {};

struct ImportRequested {
    std::string staged_path;
    std::string source_filename;
    std::uint64_t added_order{0};
};

struct ImportStarted {};

struct ImportDuplicate {
    std::string source_filename;
};

struct ImportFailed {
    std::string source_filename;
};



struct BookOpened {
    BookId book_id;
};

struct BookOpenFailed {
    BookId book_id;
};

struct PageNextRequested {};
struct PagePreviousRequested {};
struct BackRequested {};
struct RemovableStorageLost {};
struct OpenReaderMenuRequested {};
struct OpenSearchRequested {};
struct OpenSettingsRequested {};
struct SettingsReadingRequested {};
struct SettingsDisplayRequested {};
struct SettingsWiFiRequested {};
struct SettingsLanguageRequested {};
struct SettingsStorageRequested {};
struct SettingsSleepRequested {};
struct SettingsAboutRequested {};
struct SettingsPowerOffRequested {};
struct SleepRequested {};
struct WakeRequested {};

struct OrientationChanged {
    Orientation orientation{Orientation::Portrait};
};

struct BookPositionChanged {
    SemanticPosition position;
};

struct ImportProgressChanged {
    std::uint8_t percent{0};
};

struct ImportCompleted {
    BookId book_id;
};

struct WiFiConnected {
    std::string ssid;
};

struct WiFiDisconnected {};

struct BatteryStateChanged {
    std::uint8_t percent{0};
    bool charging{false};
};

struct ErrorReported {
    AppError error;
};

struct RecoveryModeRequested {};

struct BootStageChanged {
    BootStage stage{BootStage::Reset};
};

struct BootStable {};

struct RefreshRequested {
    RefreshRequest request;
};

struct RefreshCompleted {
    std::uint32_t generation{0};
};

struct RefreshFailed {
    std::uint32_t generation{0};
};

using AppEvent = std::variant<
    InputReceived,
    ActionRequested,
    OpenBookRequested,
    LibraryRefreshRequested,
    OpenLibrarySearchRequested,
    LibraryFilterChanged,
    LibrarySortChanged,
    LibraryViewChanged,
    LocaleChanged,
    TypographyDefaultsChanged,
    WiFiPolicyChanged,
    LibrarySearchChanged,
    LibraryFocusNextRequested,
    LibraryFocusPreviousRequested,
    OpenFocusedBookRequested,
    OpenFocusedBookDetailsRequested,
    RestartFocusedBookRequested,
    RemoveFocusedBookRequested,
    DeleteFocusedBookRequested,
    ImportRequested,
    ImportStarted,
    ImportDuplicate,
    ImportFailed,
    BookOpened,
    BookOpenFailed,
    PageNextRequested,
    PagePreviousRequested,
    BackRequested,
    RemovableStorageLost,
    OpenReaderMenuRequested,
    OpenSearchRequested,
    OpenSettingsRequested,
    SettingsReadingRequested,
    SettingsDisplayRequested,
    SettingsWiFiRequested,
    SettingsLanguageRequested,
    SettingsStorageRequested,
    SettingsSleepRequested,
    SettingsAboutRequested,
    SettingsPowerOffRequested,
    SleepRequested,
    WakeRequested,
    OrientationChanged,
    BookPositionChanged,
    ImportProgressChanged,
    ImportCompleted,
    WiFiConnected,
    WiFiDisconnected,
    BatteryStateChanged,
    ErrorReported,
    RecoveryModeRequested,
    BootStageChanged,
    BootStable,
    RefreshRequested,
    RefreshCompleted,
    RefreshFailed
>;

} // namespace enku
