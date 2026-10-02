#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../render/settings_screen_renderer.hpp"
#include "../services/services.hpp"
#include "library_runtime.hpp"

namespace enku {

enum class SettingsNavigationResult : std::uint8_t {
    Ignored,
    Applied,
    ReadingRequested,
    DisplayRequested,
    WiFiRequested,
    LanguageRequested,
    StorageRequested,
    SleepRequested,
    AboutRequested,
    PowerOffRequested,
    Failed,
};

class SettingsNavigationRuntime {
public:
    SettingsNavigationRuntime(
        AppState& app_state,
        LibraryRuntimeController& library,
        SettingsScreenRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    SettingsNavigationResult handle(
        const OpenSettingsRequested&
    );

    SettingsNavigationResult handle(
        LogicalAction action
    );

    SettingsNavigationResult resume();

private:
    AppState& app_state_;
    LibraryRuntimeController& library_;
    SettingsScreenRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    SettingsNavigationResult open();
    SettingsNavigationResult close();
    SettingsNavigationResult render();
    SettingsNavigationResult moveFocus(int direction);
    SettingsNavigationResult activate();
};

} // namespace enku
