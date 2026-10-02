#include "enku/platform/esp_idf/esp_idf_network_service.hpp"

#include <cstdint>

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_WIFI";

} // namespace

EspIdfNetworkService::~EspIdfNetworkService() {
    if (wifi_handler_ != nullptr) {
        esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_handler_
        );
        wifi_handler_ = nullptr;
    }

    if (initialized_) {
        esp_wifi_stop();
        esp_wifi_deinit();
        initialized_ = false;
    }

    if (station_netif_ != nullptr) {
        esp_netif_destroy(station_netif_);
        station_netif_ = nullptr;
    }
}

bool EspIdfNetworkService::begin() {
    if (initialized_) {
        return true;
    }

    esp_err_t result = nvs_flash_init();

    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        result = nvs_flash_init();
    }

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "NVS init failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    result = esp_netif_init();
    if (result != ESP_OK &&
        result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(
            kTag,
            "esp_netif_init failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    result = esp_event_loop_create_default();
    if (result != ESP_OK &&
        result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(
            kTag,
            "event loop init failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    station_netif_ =
        esp_netif_create_default_wifi_sta();

    if (station_netif_ == nullptr) {
        ESP_LOGE(kTag, "Wi-Fi STA netif creation failed");
        return false;
    }

    wifi_init_config_t config =
        WIFI_INIT_CONFIG_DEFAULT();

    result = esp_wifi_init(&config);
    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "esp_wifi_init failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    result =
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &EspIdfNetworkService::handleWifiEvent,
            this,
            &wifi_handler_
        );

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "Wi-Fi event registration failed: %s",
            esp_err_to_name(result)
        );
        esp_wifi_deinit();
        return false;
    }

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        ESP_LOGE(kTag, "Wi-Fi STA start failed");
        return false;
    }

    initialized_ = true;
    connected_.store(false);

    ESP_LOGI(
        kTag,
        "Wi-Fi STA stack ready; auto-connect disabled"
    );

    return true;
}

bool EspIdfNetworkService::connected() const {
    return connected_.load();
}

void EspIdfNetworkService::disconnect() {
    connected_.store(false);

    if (initialized_) {
        esp_wifi_disconnect();
    }
}

void EspIdfNetworkService::handleWifiEvent(
    void* arg,
    esp_event_base_t,
    std::int32_t event_id,
    void*
) {
    auto* self =
        static_cast<EspIdfNetworkService*>(arg);

    if (self == nullptr) {
        return;
    }

    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        self->connected_.store(true);
    } else if (
        event_id == WIFI_EVENT_STA_DISCONNECTED ||
        event_id == WIFI_EVENT_STA_STOP
    ) {
        self->connected_.store(false);
    }
}

} // namespace enku::platform::esp_idf
