#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../services/services.hpp"

namespace enku {

enum class NetworkLifecycleResult : std::uint8_t {
    Applied,
    NoTrustedNetwork,
    Failed,
};

class NetworkLifecycleCoordinator {
public:
    NetworkLifecycleCoordinator(
        AppState& app_state,
        NetworkService& network,
        NetworkSettingsService& network_settings
    );

    NetworkLifecycleResult applyPolicy();
    NetworkLifecycleResult resume();
    void disconnectForSuspend();
    void sync();

    NetworkPolicyStatus lastPolicyStatus() const;

private:
    AppState& app_state_;
    NetworkService& network_;
    NetworkSettingsService& network_settings_;
    NetworkPolicyStatus last_policy_status_{
        NetworkPolicyStatus::Ok
    };

    void updateStatus();
};

} // namespace enku
