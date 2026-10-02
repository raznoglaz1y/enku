#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "esp_event.h"
#include "esp_netif.h"

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

    NetworkPolicyStatus applyPolicy(
        WiFiPolicy policy
    ) override;

    NetworkPolicyStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view password
    ) override;

    NetworkPolicyStatus forgetTrustedNetwork() override;

    std::optional<std::string> trustedSsid() const override;

private:
    std::atomic_bool connected_{false};
    std::atomic_bool started_{false};
    bool initialized_{false};
    esp_netif_t* station_netif_{nullptr};
    esp_event_handler_instance_t wifi_handler_{nullptr};

    bool ensureStarted();

    static void handleWifiEvent(
        void* arg,
        esp_event_base_t event_base,
        std::int32_t event_id,
        void* event_data
    );
};

} // namespace enku::platform::esp_idf
