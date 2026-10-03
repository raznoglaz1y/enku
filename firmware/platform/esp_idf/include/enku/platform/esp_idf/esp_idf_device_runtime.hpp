#pragma once

#include <cstdint>
#include <string_view>

#include "enku/runtime/application_reader_runtime.hpp"
#include "enku/runtime/application_storage_runtime.hpp"
#include "enku/runtime/input_dispatcher.hpp"
#include "enku/runtime/library_search_runtime.hpp"
#include "enku/runtime/book_details_runtime.hpp"
#include "enku/runtime/book_finished_runtime.hpp"
#include "enku/runtime/contents_bookmarks_runtime.hpp"
#include "enku/runtime/about_book_runtime.hpp"
#include "enku/runtime/settings_navigation_runtime.hpp"
#include "enku/runtime/reading_settings_runtime.hpp"
#include "enku/runtime/display_settings_runtime.hpp"
#include "enku/runtime/locale_settings_runtime.hpp"
#include "enku/runtime/about_device_runtime.hpp"
#include "enku/runtime/power_off_confirm_runtime.hpp"
#include "enku/runtime/wifi_settings_runtime.hpp"
#include "enku/runtime/power_off.hpp"
#include "enku/runtime/reader_overlay_runtime.hpp"
#include "enku/runtime/search_runtime.hpp"
#include "enku/runtime/sleep_wake.hpp"
#include "enku/runtime/network_lifecycle.hpp"

#include "esp_idf_platform.hpp"
#include "freetype_text_renderer.hpp"
#include "esp_idf_web_upload_server.hpp"

namespace enku::platform::esp_idf {

enum class DeviceRuntimeInitStatus : std::uint8_t {
    Ok,
    BoardProfileMismatch,
    FontMissing,
    FontInitFailed,
    NetworkPolicyFailed,
    RecoveryRequired,
};

enum class DeviceNetworkUpdateStatus : std::uint8_t {
    Ok,
    NoTrustedNetwork,
    InvalidCredentials,
    SettingsSaveFailed,
    DriverError,
};

class EspIdfDeviceRuntime {
public:
    EspIdfDeviceRuntime(
        EspIdfPlatform& platform,
        TypographySettings typography,
        Viewport viewport
    );

    DeviceRuntimeInitStatus begin();

    ApplicationStorageRuntime& storage();
    ApplicationReaderRuntime& reader();
    FreeTypeTextRenderer& textRenderer();
    LibrarySearchRuntime& librarySearch();
    BookDetailsRuntime& bookDetails();
    BookFinishedRuntime& bookFinished();
    ContentsBookmarksRuntime& contentsBookmarks();
    AboutBookRuntime& aboutBook();
    SettingsNavigationRuntime& settingsNavigation();
    ReadingSettingsRuntime& readingSettings();
    DisplaySettingsRuntime& displaySettings();
    LocaleSettingsRuntime& localeSettings();
    AboutDeviceRuntime& aboutDevice();
    PowerOffConfirmRuntime& powerOffConfirm();
    WiFiSettingsRuntime& wifiSettings();

    SleepWakeCoordinator& sleepWake();
    PowerOffCoordinator& powerOff();
    InputDispatcher& input();

    InputDispatchResult pollInput(
        std::uint32_t now_ms
    );

    NetworkPolicyStatus applyNetworkPolicy();

    DeviceNetworkUpdateStatus setWiFiPolicy(
        WiFiPolicy policy
    );

    DeviceNetworkUpdateStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view password
    );

    DeviceNetworkUpdateStatus forgetTrustedNetwork();

    void syncPlatformState();

    const BoardProfile& boardProfile() const;
    bool supports(BoardFeature feature) const;

    const BootRestoreResult& bootResult() const;

private:
    bool refreshStatusBarIfNeeded();
    bool syncWebUploadServer();
    bool syncRemovableStorage(std::uint32_t now_ms);
    void processWebDeleteRequests();
    void refreshLibraryAfterUploadIfNeeded();


    EspIdfPlatform& platform_;

    ApplicationStorageRuntime storage_;
    FreeTypeTextRenderer text_renderer_;
    ApplicationReaderRuntime reader_;
    ReaderOverlayRuntime reader_overlay_;
    SearchRuntime search_;
    LibrarySearchRuntime library_search_;
    BookDetailsRuntime book_details_;
    BookFinishedRuntime book_finished_;
    ContentsBookmarksRuntime contents_bookmarks_;
    AboutBookRuntime about_book_;
    SettingsNavigationRuntime settings_nav_;
    ReadingSettingsRuntime reading_settings_;
    DisplaySettingsRuntime display_settings_;
    LocaleSettingsRuntime locale_settings_;
    AboutDeviceRuntime about_device_;
    NetworkLifecycleCoordinator network_lifecycle_;
    EspIdfWebUploadServer web_upload_server_;

    SleepWakeCoordinator sleep_wake_;
    PowerOffCoordinator power_off_;
    PowerOffConfirmRuntime power_off_confirm_;
    WiFiSettingsRuntime wifi_settings_;
    InputDispatcher input_dispatcher_;

    NetworkPolicyStatus network_policy_status_{
        NetworkPolicyStatus::Ok
    };
    BootRestoreResult boot_result_{};

    bool status_snapshot_valid_{false};
    NetworkRuntimeStatus rendered_network_status_{
        NetworkRuntimeStatus::Idle
    };
    std::uint8_t rendered_battery_percent_{0};
    bool rendered_charging_{false};
    Orientation rendered_orientation_{
        Orientation::Portrait
    };
    bool library_refresh_pending_{false};
    std::uint32_t last_storage_retry_ms_{0};
};

} // namespace enku::platform::esp_idf
