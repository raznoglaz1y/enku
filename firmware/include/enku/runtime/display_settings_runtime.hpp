#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/display_settings_renderer.hpp"
#include "../services/services.hpp"
#include "application_reader_runtime.hpp"
#include "settings_navigation_runtime.hpp"

namespace enku {

enum class DisplaySettingsRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class DisplaySettingsRuntime {
public:
    DisplaySettingsRuntime(
        AppState& app_state,
        ApplicationReaderRuntime& reader,
        SettingsNavigationRuntime& settings_nav,
        DisplaySettingsRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    DisplaySettingsRuntimeResult openFromSettings();

    DisplaySettingsRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    ApplicationReaderRuntime& reader_;
    SettingsNavigationRuntime& settings_nav_;
    DisplaySettingsRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    DisplaySettingsRuntimeResult render();
    DisplaySettingsRuntimeResult close();
    DisplaySettingsRuntimeResult toggleSelection();
    DisplaySettingsRuntimeResult applySelection();
};

} // namespace enku
