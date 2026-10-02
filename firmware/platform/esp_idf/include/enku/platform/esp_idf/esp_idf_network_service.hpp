#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include "enku/services/services.hpp"

namespace enku::platform::esp_idf {

class EspIdfNetworkService final
    : public NetworkService,
      public NetworkSettingsService {
public:
    EspIdfNetworkService() = default;
    ~EspIdfNetworkService();

    bool begin();

    bool connected() const override;
    void disconnect() override;
    NetworkLinkState connectionState() const override;

    NetworkPolicyStatus applyPolicy(
        WiFiPolicy policy
    ) override;

    NetworkPolicyStatus scanNetworks(
        std::vector<WiFiNetworkInfo>& networks
    ) override;

    NetworkPolicyStatus connectToNetwork(
        std::string_view ssid,
        std::string_view password
    ) override;

    NetworkPolicyStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view password
    ) override;

    NetworkPolicyStatus forgetTrustedNetwork() override;

    std::optional<std::string> trustedSsid() const override;

private:
    std::atomic_bool connected_{false};
    std::atomic<NetworkLinkState> link_state_{NetworkLinkState::Disconnected};
    std::atomic_bool started_{false};
    std::atomic_bool manual_disconnect_{false};
    std::atomic_bool transient_connect_{false};
    std::atomic<WiFiPolicy> active_policy_{WiFiPolicy::Off};
    std::atomic_uint8_t reconnect_attempt_{0};
    bool initialized_{false};
    esp_netif_t* station_netif_{nullptr};
    esp_event_handler_instance_t wifi_handler_{nullptr};
    esp_event_handler_instance_t ip_handler_{nullptr};
    esp_timer_handle_t reconnect_timer_{nullptr};

    bool ensureStarted();
    void resetReconnect();
    void scheduleReconnect();

    static void handleWifiEvent(
        void* arg,
        esp_event_base_t event_base,
        std::int32_t event_id,
        void* event_data
    );

    static void handleIpEvent(
        void* arg,
        esp_event_base_t event_base,
        std::int32_t event_id,
        void* event_data
    );

    static void reconnectTimerCallback(
        void* arg
    );
};

} // namespace enku::platform::esp_idf
