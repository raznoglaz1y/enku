#include "enku/runtime/network_lifecycle.hpp"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace enku;

namespace {

class FakeNetwork final
    : public NetworkService,
      public NetworkSettingsService {
public:
    bool connected() const override {
        return connected_;
    }

    void disconnect() override {
        connected_ = false;
        link_state = NetworkLinkState::Disconnected;
        ++disconnects;
    }

    NetworkLinkState connectionState() const override {
        return link_state;
    }

    std::optional<std::string> localAddress() const override {
        return address;
    }

    NetworkPolicyStatus applyPolicy(
        WiFiPolicy policy
    ) override {
        ++apply_calls;
        last_policy = policy;

        if (forced_status != NetworkPolicyStatus::Ok) {
            connected_ = false;
            link_state = NetworkLinkState::Failed;
            return forced_status;
        }

        if (policy == WiFiPolicy::Off ||
            policy == WiFiPolicy::Manual) {
            connected_ = false;
            link_state = NetworkLinkState::Disconnected;
            return NetworkPolicyStatus::Ok;
        }

        if (!trusted.has_value()) {
            connected_ = false;
            link_state = NetworkLinkState::Disconnected;
            return NetworkPolicyStatus::NoTrustedNetwork;
        }

        connected_ = connect_immediately;
        link_state = connect_immediately
            ? NetworkLinkState::Online
            : NetworkLinkState::Connecting;
        return NetworkPolicyStatus::Ok;
    }

    NetworkPolicyStatus scanNetworks(
        std::vector<WiFiNetworkInfo>&
    ) override {
        return NetworkPolicyStatus::Ok;
    }

    NetworkPolicyStatus connectToNetwork(
        std::string_view,
        std::string_view
    ) override {
        return NetworkPolicyStatus::Ok;
    }

    NetworkPolicyStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view
    ) override {
        trusted = std::string(ssid);
        return NetworkPolicyStatus::Ok;
    }

    NetworkPolicyStatus forgetTrustedNetwork() override {
        trusted.reset();
        connected_ = false;
        return NetworkPolicyStatus::Ok;
    }

    std::optional<std::string> trustedSsid() const override {
        return trusted;
    }

    bool connected_{false};
    bool connect_immediately{false};
    NetworkLinkState link_state{NetworkLinkState::Disconnected};
    std::uint32_t disconnects{0};
    std::uint32_t apply_calls{0};
    WiFiPolicy last_policy{WiFiPolicy::Manual};
    NetworkPolicyStatus forced_status{NetworkPolicyStatus::Ok};
    std::optional<std::string> trusted;
    std::optional<std::string> address;
};

} // namespace

int main() {
    AppState app;
    FakeNetwork network;

    NetworkLifecycleCoordinator lifecycle(
        app,
        network,
        network
    );

    app.wifi_policy = WiFiPolicy::Off;
    assert(
        lifecycle.applyPolicy() ==
        NetworkLifecycleResult::Applied
    );
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Off
    );
    assert(!app.network.connected);

    app.wifi_policy = WiFiPolicy::Manual;
    assert(
        lifecycle.applyPolicy() ==
        NetworkLifecycleResult::Applied
    );
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Idle
    );

    app.wifi_policy = WiFiPolicy::AutoConnectTrusted;
    network.trusted.reset();
    assert(
        lifecycle.applyPolicy() ==
        NetworkLifecycleResult::NoTrustedNetwork
    );
    assert(
        app.network.status ==
        NetworkRuntimeStatus::NoTrustedNetwork
    );
    assert(app.network.ssid.empty());

    network.trusted = std::string{"Home"};
    network.connect_immediately = false;
    assert(
        lifecycle.applyPolicy() ==
        NetworkLifecycleResult::Applied
    );
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Connecting
    );
    assert(app.network.ssid == "Home");

    network.connected_ = true;
    network.link_state = NetworkLinkState::Online;
    network.address = std::string{"192.168.1.42"};
    lifecycle.sync();
    assert(app.network.connected);
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Connected
    );
    assert(
        app.network.address ==
        "192.168.1.42"
    );

    lifecycle.disconnectForSuspend();
    assert(!app.network.connected);
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Idle
    );
    network.address.reset();
    lifecycle.sync();
    assert(app.network.address.empty());

    network.connect_immediately = true;
    assert(
        lifecycle.resume() ==
        NetworkLifecycleResult::Applied
    );
    assert(app.network.connected);
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Connected
    );

    network.forced_status =
        NetworkPolicyStatus::DriverError;
    assert(
        lifecycle.applyPolicy() ==
        NetworkLifecycleResult::Failed
    );
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Error
    );

    // A later asynchronous link failure must propagate even when the
    // original policy application itself returned Ok.
    network.forced_status = NetworkPolicyStatus::Ok;
    network.link_state = NetworkLinkState::Failed;
    network.connected_ = false;
    lifecycle.sync();
    assert(
        app.network.status ==
        NetworkRuntimeStatus::Error
    );

    return 0;
}
