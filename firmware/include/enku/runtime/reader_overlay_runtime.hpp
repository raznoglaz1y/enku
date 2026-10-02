#pragma once

#include <cstdint>

#include "../core/input.hpp"
#include "../core/settings.hpp"
#include "../render/reader_overlay_renderer.hpp"
#include "../services/services.hpp"
#include "application_reader_runtime.hpp"
#include "application_storage_runtime.hpp"

namespace enku {

enum class ReaderOverlayRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class ReaderOverlayRuntime {
public:
    ReaderOverlayRuntime(
        ApplicationStorageRuntime& storage,
        ApplicationReaderRuntime& reader,
        ReaderOverlayRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    ReaderOverlayRuntimeResult handle(
        LogicalAction action
    );

private:
    ApplicationStorageRuntime& storage_;
    ApplicationReaderRuntime& reader_;
    ReaderOverlayRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};

    GlobalSettings baseline_{};
    bool baseline_valid_{false};

    ReaderOverlayRuntimeResult openMenu();
    ReaderOverlayRuntimeResult openQuickTypography();
    ReaderOverlayRuntimeResult navigate(int direction);
    ReaderOverlayRuntimeResult confirm();
    ReaderOverlayRuntimeResult close();

    ReaderOverlayRuntimeResult previewPreset(
        ReadingPreset preset
    );

    ReaderOverlayRuntimeResult restoreBaseline();
    ReaderOverlayRuntimeResult renderOverlay();
    ReaderOverlayRuntimeResult refreshCurrentFrame();
    void enterQuickTypography();
};

} // namespace enku
