#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/wifi_settings_renderer.hpp"
#include "../services/services.hpp"
#include "application_storage_runtime.hpp"
#include "settings_navigation_runtime.hpp"
#include "keyboard_runtime.hpp"

namespace enku {

enum class WiFiSettingsRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    NoTrustedNetwork,
    Failed,
};

class WiFiSettingsRuntime {
public:
    WiFiSettingsRuntime(
        AppState& app_state,
        ApplicationStorageRuntime& storage,
        NetworkService& network,
        NetworkSettingsService& network_settings,
        SettingsNavigationRuntime& settings_nav,
        WiFiSettingsRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    WiFiSettingsRuntimeResult openFromSettings();

    WiFiSettingsRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    ApplicationStorageRuntime& storage_;
    NetworkService& network_;
    NetworkSettingsService& network_settings_;
    SettingsNavigationRuntime& settings_nav_;
    WiFiSettingsRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    KeyboardRuntime keyboard_;
    std::uint32_t refresh_generation_{0};

    WiFiSettingsRuntimeResult render();
    WiFiSettingsRuntimeResult close();
    WiFiSettingsRuntimeResult moveFocus(int direction);
    WiFiSettingsRuntimeResult scanNetworks();
    WiFiSettingsRuntimeResult moveNetworkFocus(int direction);
    WiFiSettingsRuntimeResult chooseNetwork();
    WiFiSettingsRuntimeResult connectPendingNetwork();
    WiFiSettingsRuntimeResult cancelNetworkFlow();
    WiFiSettingsRuntimeResult beginPolicyEdit();
    WiFiSettingsRuntimeResult cyclePolicy(int direction);
    WiFiSettingsRuntimeResult applyPolicy();
    WiFiSettingsRuntimeResult cancelPolicyEdit();
    WiFiSettingsRuntimeResult enterForgetConfirm();
    WiFiSettingsRuntimeResult confirmForget();
    WiFiSettingsRuntimeResult cancelForget();
    void syncNetworkState();
};

} // namespace enku
