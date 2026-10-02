#include "enku/platform/esp_idf/esp_idf_device_runtime.hpp"

namespace enku::platform::esp_idf {

EspIdfDeviceRuntime::EspIdfDeviceRuntime(
    EspIdfPlatform& platform,
    TypographySettings typography,
    Viewport viewport
)
    : platform_(platform),
      storage_(
          platform_.stateFiles(),
          platform_.bookFiles()
      ),
      text_renderer_(
          platform_.framebuffer()
      ),
      reader_(
          storage_,
          platform_.refresh(),
          text_renderer_,
          text_renderer_,
          text_renderer_,
          typography,
          viewport
      ),
      sleep_wake_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          platform_.network(),
          platform_.power(),
          reader_.bootRestore()
      ),
      power_off_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          platform_.network(),
          platform_.power()
      ),
      input_dispatcher_(
          storage_.appState(),
          reader_.library(),
          reader_.reader(),
          sleep_wake_,
          power_off_
      ) {}

DeviceRuntimeInitStatus
EspIdfDeviceRuntime::begin() {
    const auto font_status =
        text_renderer_.begin(
            storage_.appState().
                typography.font_size_px
        );

    if (font_status ==
        FontInitStatus::FontNotFound) {
        return DeviceRuntimeInitStatus::FontMissing;
    }

    if (font_status != FontInitStatus::Ok) {
        return DeviceRuntimeInitStatus::FontInitFailed;
    }

    boot_result_ = reader_.bootRestore().run();

    if (boot_result_.status ==
        BootRestoreStatus::RecoveryRequired) {
        return DeviceRuntimeInitStatus::RecoveryRequired;
    }

    network_policy_status_ =
        applyNetworkPolicy();

    if (network_policy_status_ ==
        NetworkPolicyStatus::DriverError) {
        return DeviceRuntimeInitStatus::NetworkPolicyFailed;
    }

    syncPlatformState();

    return DeviceRuntimeInitStatus::Ok;
}

ApplicationStorageRuntime&
EspIdfDeviceRuntime::storage() {
    return storage_;
}

ApplicationReaderRuntime&
EspIdfDeviceRuntime::reader() {
    return reader_;
}

FreeTypeTextRenderer&
EspIdfDeviceRuntime::textRenderer() {
    return text_renderer_;
}

SleepWakeCoordinator&
EspIdfDeviceRuntime::sleepWake() {
    return sleep_wake_;
}

PowerOffCoordinator&
EspIdfDeviceRuntime::powerOff() {
    return power_off_;
}

InputDispatcher&
EspIdfDeviceRuntime::input() {
    return input_dispatcher_;
}

NetworkPolicyStatus
EspIdfDeviceRuntime::applyNetworkPolicy() {
    const auto status =
        platform_.network().applyPolicy(
            storage_.appState().wifi_policy
        );

    network_policy_status_ = status;
    syncPlatformState();
    return status;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::setWiFiPolicy(
    WiFiPolicy policy
) {
    auto& app = storage_.appState();
    const auto previous = app.wifi_policy;

    if (storage_.settingsRuntime().handle(
            WiFiPolicyChanged{policy}
        ) != PersistStatus::Ok) {
        return DeviceNetworkUpdateStatus::SettingsSaveFailed;
    }

    const auto status =
        platform_.network().applyPolicy(policy);

    if (status == NetworkPolicyStatus::DriverError) {
        // Best-effort rollback keeps durable policy aligned with hardware.
        storage_.settingsRuntime().handle(
            WiFiPolicyChanged{previous}
        );
        platform_.network().applyPolicy(previous);
        syncPlatformState();
        return DeviceNetworkUpdateStatus::DriverError;
    }

    syncPlatformState();

    if (status ==
        NetworkPolicyStatus::NoTrustedNetwork) {
        return DeviceNetworkUpdateStatus::NoTrustedNetwork;
    }

    return DeviceNetworkUpdateStatus::Ok;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::setTrustedNetwork(
    std::string_view ssid,
    std::string_view password
) {
    const auto stored =
        platform_.network().setTrustedNetwork(
            ssid,
            password
        );

    if (stored ==
        NetworkPolicyStatus::InvalidCredentials) {
        return DeviceNetworkUpdateStatus::InvalidCredentials;
    }

    if (stored != NetworkPolicyStatus::Ok) {
        return DeviceNetworkUpdateStatus::DriverError;
    }

    if (storage_.appState().wifi_policy ==
        WiFiPolicy::AutoConnectTrusted) {
        const auto applied =
            platform_.network().applyPolicy(
                WiFiPolicy::AutoConnectTrusted
            );

        if (applied == NetworkPolicyStatus::DriverError) {
            return DeviceNetworkUpdateStatus::DriverError;
        }
    }

    syncPlatformState();
    return DeviceNetworkUpdateStatus::Ok;
}

DeviceNetworkUpdateStatus
EspIdfDeviceRuntime::forgetTrustedNetwork() {
    const auto status =
        platform_.network().forgetTrustedNetwork();

    if (status != NetworkPolicyStatus::Ok) {
        return DeviceNetworkUpdateStatus::DriverError;
    }

    syncPlatformState();
    return DeviceNetworkUpdateStatus::Ok;
}

void EspIdfDeviceRuntime::syncPlatformState() {
    auto& app = storage_.appState();

    app.network.connected =
        platform_.network().connected();

    const auto ssid =
        platform_.network().trustedSsid();

    app.network.ssid =
        ssid.has_value()
            ? *ssid
            : std::string{};

    const auto battery =
        platform_.power().batteryState();

    app.power.battery_percent =
        battery.percent;
    app.power.charging =
        battery.charging;
}

InputDispatchResult EspIdfDeviceRuntime::pollInput(
    std::uint32_t now_ms
) {
    syncPlatformState();

    const auto event =
        platform_.buttons().poll(now_ms);

    if (!event.has_value()) {
        return InputDispatchResult::Ignored;
    }

    return input_dispatcher_.handle(*event);
}

const BootRestoreResult&
EspIdfDeviceRuntime::bootResult() const {
    return boot_result_;
}

} // namespace enku::platform::esp_idf
