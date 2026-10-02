#include "enku/runtime/network_lifecycle.hpp"

namespace enku {

NetworkLifecycleCoordinator::NetworkLifecycleCoordinator(
    AppState& app_state,
    NetworkService& network,
    NetworkSettingsService& network_settings
)
    : app_state_(app_state),
      network_(network),
      network_settings_(network_settings) {}

NetworkLifecycleResult
NetworkLifecycleCoordinator::applyPolicy() {
    last_policy_status_ =
        network_settings_.applyPolicy(
            app_state_.wifi_policy
        );

    updateStatus();

    if (last_policy_status_ ==
        NetworkPolicyStatus::NoTrustedNetwork) {
        return NetworkLifecycleResult::NoTrustedNetwork;
    }

    if (last_policy_status_ ==
            NetworkPolicyStatus::DriverError ||
        last_policy_status_ ==
            NetworkPolicyStatus::InvalidCredentials ||
        last_policy_status_ ==
            NetworkPolicyStatus::ConnectionFailed) {
        return NetworkLifecycleResult::Failed;
    }

    return NetworkLifecycleResult::Applied;
}

NetworkLifecycleResult
NetworkLifecycleCoordinator::resume() {
    return applyPolicy();
}

void NetworkLifecycleCoordinator::disconnectForSuspend() {
    if (network_.connected()) {
        network_.disconnect();
    }

    app_state_.network.connected = false;

    if (app_state_.wifi_policy == WiFiPolicy::Off) {
        app_state_.network.status =
            NetworkRuntimeStatus::Off;
    } else {
        app_state_.network.status =
            NetworkRuntimeStatus::Idle;
    }
}

void NetworkLifecycleCoordinator::sync() {
    updateStatus();
}

NetworkPolicyStatus
NetworkLifecycleCoordinator::lastPolicyStatus() const {
    return last_policy_status_;
}

void NetworkLifecycleCoordinator::updateStatus() {
    app_state_.network.connected =
        network_.connected();

    const auto trusted =
        network_settings_.trustedSsid();

    app_state_.network.ssid =
        trusted.has_value()
            ? *trusted
            : std::string{};

    if (app_state_.network.connected) {
        app_state_.network.status =
            NetworkRuntimeStatus::Connected;
        return;
    }

    if (last_policy_status_ ==
            NetworkPolicyStatus::DriverError ||
        last_policy_status_ ==
            NetworkPolicyStatus::InvalidCredentials ||
        last_policy_status_ ==
            NetworkPolicyStatus::ConnectionFailed) {
        app_state_.network.status =
            NetworkRuntimeStatus::Error;
        return;
    }

    if (app_state_.wifi_policy == WiFiPolicy::Off) {
        app_state_.network.status =
            NetworkRuntimeStatus::Off;
        return;
    }

    if (last_policy_status_ ==
        NetworkPolicyStatus::NoTrustedNetwork) {
        app_state_.network.status =
            NetworkRuntimeStatus::NoTrustedNetwork;
        return;
    }

    if (app_state_.wifi_policy ==
        WiFiPolicy::AutoConnectTrusted) {
        app_state_.network.status =
            NetworkRuntimeStatus::Connecting;
        return;
    }

    app_state_.network.status =
        NetworkRuntimeStatus::Idle;
}

} // namespace enku
