#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/reading_settings_renderer.hpp"
#include "../services/services.hpp"
#include "application_reader_runtime.hpp"
#include "application_storage_runtime.hpp"
#include "settings_navigation_runtime.hpp"

namespace enku {

enum class ReadingSettingsRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class ReadingSettingsRuntime {
public:
    ReadingSettingsRuntime(
        AppState& app_state,
        ApplicationStorageRuntime& storage,
        ApplicationReaderRuntime& reader,
        SettingsNavigationRuntime& settings_nav,
        ReadingSettingsRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    ReadingSettingsRuntimeResult openFromSettings();

    ReadingSettingsRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    ApplicationStorageRuntime& storage_;
    ApplicationReaderRuntime& reader_;
    SettingsNavigationRuntime& settings_nav_;
    ReadingSettingsRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    ReadingSettingsRuntimeResult render();
    ReadingSettingsRuntimeResult close();
    ReadingSettingsRuntimeResult moveFocus(int direction);
    ReadingSettingsRuntimeResult beginEdit();
    ReadingSettingsRuntimeResult adjust(int direction);
    ReadingSettingsRuntimeResult commitEdit();
    ReadingSettingsRuntimeResult cancelEdit();

    bool applyPreview(
        const TypographyState& typography
    );

    static ReadingPreset nextPreset(
        ReadingPreset current,
        int direction
    );
};

} // namespace enku
