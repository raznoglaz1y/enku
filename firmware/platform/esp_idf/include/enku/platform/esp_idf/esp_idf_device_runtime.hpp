#pragma once

#include <cstdint>
#include <string_view>

#include "enku/runtime/application_reader_runtime.hpp"
#include "enku/runtime/application_storage_runtime.hpp"
#include "enku/runtime/input_dispatcher.hpp"
#include "enku/runtime/library_search_runtime.hpp"
#include "enku/runtime/power_off.hpp"
#include "enku/runtime/reader_overlay_runtime.hpp"
#include "enku/runtime/search_runtime.hpp"
#include "enku/runtime/sleep_wake.hpp"

#include "esp_idf_platform.hpp"
#include "freetype_text_renderer.hpp"

namespace enku::platform::esp_idf {

enum class DeviceRuntimeInitStatus : std::uint8_t {
    Ok,
    FontMissing,
    FontInitFailed,
    NetworkPolicyFailed,
    RecoveryRequired,
};

enum class DeviceNetworkUpdateStatus : std::uint8_t {
    Ok,
    NoTrustedNetwork,
    InvalidCredentials,
    SettingsSaveFailed,
    DriverError,
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
    LibrarySearchRuntime& librarySearch();

    SleepWakeCoordinator& sleepWake();
    PowerOffCoordinator& powerOff();
    InputDispatcher& input();

    InputDispatchResult pollInput(
        std::uint32_t now_ms
    );

    NetworkPolicyStatus applyNetworkPolicy();

    DeviceNetworkUpdateStatus setWiFiPolicy(
        WiFiPolicy policy
    );

    DeviceNetworkUpdateStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view password
    );

    DeviceNetworkUpdateStatus forgetTrustedNetwork();

    void syncPlatformState();

    const BootRestoreResult& bootResult() const;

private:
    EspIdfPlatform& platform_;

    ApplicationStorageRuntime storage_;
    FreeTypeTextRenderer text_renderer_;
    ApplicationReaderRuntime reader_;
    ReaderOverlayRuntime reader_overlay_;
    SearchRuntime search_;
    LibrarySearchRuntime library_search_;

    SleepWakeCoordinator sleep_wake_;
    PowerOffCoordinator power_off_;
    InputDispatcher input_dispatcher_;

    NetworkPolicyStatus network_policy_status_{
        NetworkPolicyStatus::Ok
    };
    BootRestoreResult boot_result_{};
};

} // namespace enku::platform::esp_idf
