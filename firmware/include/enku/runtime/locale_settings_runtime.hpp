#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/locale_settings_renderer.hpp"
#include "../services/services.hpp"
#include "application_storage_runtime.hpp"
#include "settings_navigation_runtime.hpp"

namespace enku {

enum class LocaleSettingsRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class LocaleSettingsRuntime {
public:
    LocaleSettingsRuntime(
        AppState& app_state,
        ApplicationStorageRuntime& storage,
        SettingsNavigationRuntime& settings_nav,
        LocaleSettingsRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    LocaleSettingsRuntimeResult openFromSettings();

    LocaleSettingsRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    ApplicationStorageRuntime& storage_;
    SettingsNavigationRuntime& settings_nav_;
    LocaleSettingsRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    LocaleSettingsRuntimeResult render();
    LocaleSettingsRuntimeResult move(int direction);
    LocaleSettingsRuntimeResult apply();
    LocaleSettingsRuntimeResult close();
    void normalizeWindow();
};

} // namespace enku
