#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/power_off_confirm_renderer.hpp"
#include "../services/services.hpp"
#include "power_off.hpp"
#include "settings_navigation_runtime.hpp"

namespace enku {

enum class PowerOffConfirmRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class PowerOffConfirmRuntime {
public:
    PowerOffConfirmRuntime(
        AppState& app_state,
        PowerOffCoordinator& power_off,
        SettingsNavigationRuntime& settings_nav,
        PowerOffConfirmRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    PowerOffConfirmRuntimeResult openFromSettings();

    PowerOffConfirmRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    PowerOffCoordinator& power_off_;
    SettingsNavigationRuntime& settings_nav_;
    PowerOffConfirmRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    PowerOffConfirmRuntimeResult render();
    PowerOffConfirmRuntimeResult close();
    PowerOffConfirmRuntimeResult toggle();
    PowerOffConfirmRuntimeResult confirm();
};

} // namespace enku
