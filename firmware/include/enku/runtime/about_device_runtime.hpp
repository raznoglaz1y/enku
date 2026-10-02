#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/about_device_renderer.hpp"
#include "../services/services.hpp"
#include "settings_navigation_runtime.hpp"

namespace enku {

enum class AboutDeviceRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class AboutDeviceRuntime {
public:
    AboutDeviceRuntime(
        AppState& app_state,
        SettingsNavigationRuntime& settings_nav,
        AboutDeviceRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    AboutDeviceRuntimeResult openFromSettings();

    AboutDeviceRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    SettingsNavigationRuntime& settings_nav_;
    AboutDeviceRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    AboutDeviceRuntimeResult render();
    AboutDeviceRuntimeResult close();
};

} // namespace enku
