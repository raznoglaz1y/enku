#pragma once

#include <cstdint>

#include "enku/runtime/application_reader_runtime.hpp"
#include "enku/runtime/application_storage_runtime.hpp"

#include "esp_idf_platform.hpp"
#include "freetype_text_renderer.hpp"

namespace enku::platform::esp_idf {

enum class DeviceRuntimeInitStatus : std::uint8_t {
    Ok,
    FontMissing,
    FontInitFailed,
    RecoveryRequired,
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

    const BootRestoreResult& bootResult() const;

private:
    EspIdfPlatform& platform_;

    ApplicationStorageRuntime storage_;
    FreeTypeTextRenderer text_renderer_;
    ApplicationReaderRuntime reader_;

    BootRestoreResult boot_result_{};
};

} // namespace enku::platform::esp_idf
