#pragma once

#include <atomic>

#include "esp_event.h"
#include "esp_netif.h"

#include "enku/services/services.hpp"

namespace enku::platform::esp_idf {

class EspIdfNetworkService final : public NetworkService {
public:
    EspIdfNetworkService() = default;
    ~EspIdfNetworkService();

    bool begin();

    bool connected() const override;
    void disconnect() override;

private:
    std::atomic_bool connected_{false};
    bool initialized_{false};
    esp_netif_t* station_netif_{nullptr};
    esp_event_handler_instance_t wifi_handler_{nullptr};

    static void handleWifiEvent(
        void* arg,
        esp_event_base_t event_base,
        std::int32_t event_id,
        void* event_data
    );
};

} // namespace enku::platform::esp_idf
