#include "enku/platform/esp_idf/esp_idf_device_runtime.hpp"

namespace enku::platform::esp_idf {

EspIdfDeviceRuntime::EspIdfDeviceRuntime(
    EspIdfPlatform& platform,
    TypographySettings typography,
    Viewport viewport
)
    : platform_(platform),
      storage_(
          platform_.stateFiles(),
          platform_.bookFiles()
      ),
      text_renderer_(
          platform_.framebuffer()
      ),
      reader_(
          storage_,
          platform_.refresh(),
          text_renderer_,
          text_renderer_,
          text_renderer_,
          typography,
          viewport
      ),
      reader_overlay_(
          storage_,
          reader_,
          &text_renderer_,
          &platform_.refresh()
      ),
      search_(
          storage_,
          reader_,
          &text_renderer_,
          &platform_.refresh()
      ),
      library_search_(
          storage_.appState(),
          reader_.library()
      ),
      book_details_(
          storage_.appState(),
          storage_.library(),
          reader_.library(),
          reader_.reader(),
          storage_.checkpoints(),
          storage_.deleteService(),
          &text_renderer_,
          &platform_.refresh()
      ),
      book_finished_(
          storage_.appState(),
          storage_.library(),
          reader_.library(),
          reader_.reader(),
          storage_.checkpoints(),
          &text_renderer_,
          &platform_.refresh()
      ),
      contents_bookmarks_(
          storage_.appState(),
          reader_,
          storage_.bookmarks(),
          &text_renderer_,
          &platform_.refresh()
      ),
      about_book_(
          storage_.appState(),
          storage_.library(),
          reader_,
          &text_renderer_,
          &platform_.refresh()
      ),
      settings_nav_(
          storage_.appState(),
          reader_.library(),
          &text_renderer_,
          &platform_.refresh()
      ),
      reading_settings_(
          storage_.appState(),
          storage_,
          reader_,
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      display_settings_(
          storage_.appState(),
          reader_,
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      locale_settings_(
          storage_.appState(),
          storage_,
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      about_device_(
          storage_.appState(),
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      network_lifecycle_(
          storage_.appState(),
          platform_.network(),
          platform_.network()
      ),
      web_upload_server_(
          storage_.webUpload(),
          storage_.library()
      ),
      sleep_wake_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          network_lifecycle_,
          platform_.power(),
          reader_.bootRestore()
      ),
      power_off_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          platform_.network(),
          platform_.power()
      ),
      power_off_confirm_(
          storage_.appState(),
          power_off_,
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      wifi_settings_(
          storage_.appState(),
          storage_,
          platform_.network(),
          platform_.network(),
          network_lifecycle_,
          settings_nav_,
          &text_renderer_,
          &platform_.refresh()
      ),
      input_dispatcher_(
          storage_.appState(),
          reader_.library(),
          reader_.reader(),
          sleep_wake_,
          power_off_,
          &reader_overlay_,
          &search_,
          &library_search_,
          &book_details_,
          &book_finished_,
          &contents_bookmarks_,
          &about_book_,
          &settings_nav_,
          &reading_settings_,
          &display_settings_,
          &locale_settings_,
          &about_device_,
          &power_off_confirm_,
          &wifi_settings_
      ) {
    text_renderer_.bindAppState(
        storage_.appState()
    );
}

DeviceRuntimeInitStatus
EspIdfDeviceRuntime::begin() {
    const auto font_status =
        text_renderer_.begin(
            storage_.appState().
                typography.font_size_px
        );

    if (font_status ==
        FontInitStatus::FontNotFound) {
        return DeviceRuntimeInitStatus::FontMissing;
    }

    if (font_status != FontInitStatus::Ok) {
        return DeviceRuntimeInitStatus::FontInitFailed;
    }

    boot_result_ = reader_.bootRestore().run();

    if (boot_result_.status ==
        BootRestoreStatus::RecoveryRequired) {
        return DeviceRuntimeInitStatus::RecoveryRequired;
    }

    network_policy_status_ =
        applyNetworkPolicy();

    if (network_policy_status_ ==
        NetworkPolicyStatus::DriverError) {
        return DeviceRuntimeInitStatus::NetworkPolicyFailed;
    }

    syncPlatformState();
    syncWebUploadServer();

    return DeviceRuntimeInitStatus::Ok;
}

ApplicationStorageRuntime&
EspIdfDeviceRuntime::storage() {
    return storage_;
}

ApplicationReaderRuntime&
EspIdfDeviceRuntime::reader() {
    return reader_;
}

FreeTypeTextRenderer&
EspIdfDeviceRuntime::textRenderer() {
    return text_renderer_;
}

LibrarySearchRuntime&
EspIdfDeviceRuntime::librarySearch() {
    return library_search_;
}

BookDetailsRuntime&
EspIdfDeviceRuntime::bookDetails() {
    return book_details_;
}

BookFinishedRuntime&
EspIdfDeviceRuntime::bookFinished() {
    return book_finished_;
}

ContentsBookmarksRuntime&
EspIdfDeviceRuntime::contentsBookmarks() {
    return contents_bookmarks_;
}

AboutBookRuntime&
EspIdfDeviceRuntime::aboutBook() {
    return about_book_;
}

SettingsNavigationRuntime&
EspIdfDeviceRuntime::settingsNavigation() {
    return settings_nav_;
}

ReadingSettingsRuntime&
EspIdfDeviceRuntime::readingSettings() {
    return reading_settings_;
}

DisplaySettingsRuntime&
EspIdfDeviceRuntime::displaySettings() {
    return display_settings_;
}

LocaleSettingsRuntime&
EspIdfDeviceRuntime::localeSettings() {
    return locale_settings_;
}

AboutDeviceRuntime&
EspIdfDeviceRuntime::aboutDevice() {
    return about_device_;
}

PowerOffConfirmRuntime&
EspIdfDeviceRuntime::powerOffConfirm() {
    return power_off_confirm_;
}

WiFiSettingsRuntime&
EspIdfDeviceRuntime::wifiSettings() {
    return wifi_settings_;
}

SleepWakeCoordinator&
EspIdfDeviceRuntime::sleepWake() {
    return sleep_wake_;
}

PowerOffCoordinator&
EspIdfDeviceRuntime::powerOff() {
    return power_off_;
}

InputDispatcher&
EspIdfDeviceRuntime::input() {
    return input_dispatcher_;
}

NetworkPolicyStatus
EspIdfDeviceRuntime::applyNetworkPolicy() {
    network_lifecycle_.applyPolicy();
    network_policy_status_ =
        network_lifecycle_.lastPolicyStatus();
    syncPlatformState();
    return network_policy_status_;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::setWiFiPolicy(
    WiFiPolicy policy
) {
    auto& app = storage_.appState();
    const auto previous = app.wifi_policy;

    if (storage_.settingsRuntime().handle(
            WiFiPolicyChanged{policy}
        ) != PersistStatus::Ok) {
        return DeviceNetworkUpdateStatus::SettingsSaveFailed;
    }

    const auto lifecycle_result =
        network_lifecycle_.applyPolicy();
    const auto status =
        network_lifecycle_.lastPolicyStatus();

    if (lifecycle_result ==
            NetworkLifecycleResult::Failed ||
        status == NetworkPolicyStatus::DriverError) {
        // Best-effort rollback keeps durable policy aligned with hardware.
        storage_.settingsRuntime().handle(
            WiFiPolicyChanged{previous}
        );
        network_lifecycle_.applyPolicy();
        syncPlatformState();
        return DeviceNetworkUpdateStatus::DriverError;
    }

    syncPlatformState();

    if (status ==
        NetworkPolicyStatus::NoTrustedNetwork) {
        return DeviceNetworkUpdateStatus::NoTrustedNetwork;
    }

    return DeviceNetworkUpdateStatus::Ok;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::setTrustedNetwork(
    std::string_view ssid,
    std::string_view password
) {
    const auto stored =
        platform_.network().setTrustedNetwork(
            ssid,
            password
        );

    if (stored ==
        NetworkPolicyStatus::InvalidCredentials) {
        return DeviceNetworkUpdateStatus::InvalidCredentials;
    }

    if (stored != NetworkPolicyStatus::Ok) {
        return DeviceNetworkUpdateStatus::DriverError;
    }

    if (storage_.appState().wifi_policy ==
        WiFiPolicy::AutoConnectTrusted) {
        const auto applied =
            network_lifecycle_.applyPolicy();

        if (applied == NetworkLifecycleResult::Failed) {
            return DeviceNetworkUpdateStatus::DriverError;
        }
    }

    syncPlatformState();
    return DeviceNetworkUpdateStatus::Ok;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::forgetTrustedNetwork() {
    const auto status =
        platform_.network().forgetTrustedNetwork();

    if (status != NetworkPolicyStatus::Ok) {
        return DeviceNetworkUpdateStatus::DriverError;
    }

    network_lifecycle_.sync();
    syncPlatformState();
    return DeviceNetworkUpdateStatus::Ok;
}

void EspIdfDeviceRuntime::syncPlatformState() {
    auto& app = storage_.appState();

    network_lifecycle_.sync();

    const auto battery =
        platform_.power().batteryState();

    app.power.battery_percent =
        battery.percent;
    app.power.charging =
        battery.charging;

    // Snapshot the thread-safe HTTP ingress activity into AppState for UI
    // and sleep policy decisions. The HTTP task never writes AppState.
    app.import_active =
        storage_.webUpload().active();
}

bool EspIdfDeviceRuntime::syncWebUploadServer() {
    const auto& app = storage_.appState();

    const bool online =
        app.network.status ==
            NetworkRuntimeStatus::Connected &&
        app.screen != Screen::Sleep;

    return web_upload_server_.sync(online);
}

bool EspIdfDeviceRuntime::refreshStatusBarIfNeeded() {
    auto& app = storage_.appState();

    if (app.screen == Screen::Sleep) {
        return true;
    }

    const bool changed =
        !status_snapshot_valid_ ||
        rendered_network_status_ !=
            app.network.status ||
        rendered_battery_percent_ !=
            app.power.battery_percent ||
        rendered_charging_ !=
            app.power.charging ||
        rendered_orientation_ !=
            app.orientation;

    if (!changed) {
        return true;
    }

    if (!text_renderer_.renderStatusBar(app)) {
        return false;
    }

    RefreshRequest request;
    request.refresh_class =
        RefreshClass::Region;
    request.reason =
        RefreshReason::StatusChanged;
    request.generation = 0;
    request.may_coalesce = false;
    request.may_defer = false;

    if (app.orientation == Orientation::Landscape) {
        request.dirty_region =
            Rect{
                0,
                0,
                EspIdfEpaper::kWidth,
                24,
            };
    } else {
        request.dirty_region =
            Rect{
                static_cast<std::uint16_t>(
                    EspIdfEpaper::kWidth - 24
                ),
                0,
                24,
                EspIdfEpaper::kHeight,
            };
    }

    if (!platform_.refresh().submit(request)) {
        return false;
    }

    rendered_network_status_ =
        app.network.status;
    rendered_battery_percent_ =
        app.power.battery_percent;
    rendered_charging_ =
        app.power.charging;
    rendered_orientation_ =
        app.orientation;
    status_snapshot_valid_ = true;

    return true;
}

InputDispatchResult EspIdfDeviceRuntime::pollInput(
    std::uint32_t now_ms
) {
    syncPlatformState();

    // Network services follow the same authoritative lifecycle state as
    // the UI. The uploader exists only while the reader is online and awake.
    syncWebUploadServer();

    // Status changes are independent of user input. Keep the e-ink update
    // constrained to the compact top bar instead of refreshing the screen.
    refreshStatusBarIfNeeded();

    const auto event =
        platform_.buttons().poll(now_ms);

    if (!event.has_value()) {
        return InputDispatchResult::Ignored;
    }

    const auto result =
        input_dispatcher_.handle(*event);

    // Sleep/wake and Wi-Fi settings can change network availability while
    // dispatching this very input. Reconcile the HTTP service immediately
    // instead of waiting for the next poll cycle.
    syncPlatformState();
    syncWebUploadServer();

    return result;
}

const BootRestoreResult&
EspIdfDeviceRuntime::bootResult() const {
    return boot_result_;
}

} // namespace enku::platform::esp_idf
