#include "enku/platform/esp_idf/esp_idf_network_service.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

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
        if (started_.load()) {
            esp_wifi_stop();
        }
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

    if (esp_wifi_set_storage(WIFI_STORAGE_FLASH) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        ESP_LOGE(kTag, "Wi-Fi STA start failed");
        return false;
    }

    initialized_ = true;
    started_.store(true);
    connected_.store(false);

    ESP_LOGI(
        kTag,
        "Wi-Fi STA stack ready; auto-connect disabled"
    );

    return true;
}

bool EspIdfNetworkService::ensureStarted() {
    if (!initialized_) {
        return false;
    }

    if (started_.load()) {
        return true;
    }

    if (esp_wifi_start() != ESP_OK) {
        return false;
    }

    started_.store(true);
    return true;
}

bool EspIdfNetworkService::connected() const {
    return connected_.load();
}

void EspIdfNetworkService::disconnect() {
    connected_.store(false);

    if (initialized_ && started_.load()) {
        esp_wifi_disconnect();
    }
}

NetworkPolicyStatus EspIdfNetworkService::applyPolicy(
    WiFiPolicy policy
) {
    if (!initialized_) {
        return NetworkPolicyStatus::DriverError;
    }

    switch (policy) {
        case WiFiPolicy::Off:
            disconnect();

            if (started_.load() &&
                esp_wifi_stop() != ESP_OK) {
                return NetworkPolicyStatus::DriverError;
            }

            started_.store(false);
            return NetworkPolicyStatus::Ok;

        case WiFiPolicy::Manual:
            if (!ensureStarted()) {
                return NetworkPolicyStatus::DriverError;
            }

            disconnect();
            return NetworkPolicyStatus::Ok;

        case WiFiPolicy::AutoConnectTrusted: {
            if (!ensureStarted()) {
                return NetworkPolicyStatus::DriverError;
            }

            wifi_config_t config = {};
            if (esp_wifi_get_config(
                    WIFI_IF_STA,
                    &config
                ) != ESP_OK) {
                return NetworkPolicyStatus::DriverError;
            }

            if (config.sta.ssid[0] == '\0') {
                return NetworkPolicyStatus::NoTrustedNetwork;
            }

            if (esp_wifi_connect() != ESP_OK) {
                return NetworkPolicyStatus::DriverError;
            }

            return NetworkPolicyStatus::Ok;
        }

        default:
            return NetworkPolicyStatus::DriverError;
    }
}

NetworkPolicyStatus EspIdfNetworkService::setTrustedNetwork(
    std::string_view ssid,
    std::string_view password
) {
    if (!initialized_ ||
        ssid.empty() ||
        ssid.size() > 32U ||
        password.size() > 64U) {
        return NetworkPolicyStatus::InvalidCredentials;
    }

    if (!ensureStarted()) {
        return NetworkPolicyStatus::DriverError;
    }

    wifi_config_t config = {};

    std::memcpy(
        config.sta.ssid,
        ssid.data(),
        ssid.size()
    );

    if (!password.empty()) {
        std::memcpy(
            config.sta.password,
            password.data(),
            password.size()
        );
    }

    if (esp_wifi_set_config(
            WIFI_IF_STA,
            &config
        ) != ESP_OK) {
        return NetworkPolicyStatus::DriverError;
    }

    ESP_LOGI(
        kTag,
        "Trusted Wi-Fi network stored: %.*s",
        static_cast<int>(ssid.size()),
        ssid.data()
    );

    return NetworkPolicyStatus::Ok;
}

NetworkPolicyStatus
EspIdfNetworkService::forgetTrustedNetwork() {
    if (!initialized_) {
        return NetworkPolicyStatus::DriverError;
    }

    disconnect();

    wifi_config_t empty = {};
    if (esp_wifi_set_config(
            WIFI_IF_STA,
            &empty
        ) != ESP_OK) {
        return NetworkPolicyStatus::DriverError;
    }

    return NetworkPolicyStatus::Ok;
}

std::optional<std::string>
EspIdfNetworkService::trustedSsid() const {
    if (!initialized_) {
        return std::nullopt;
    }

    wifi_config_t config = {};
    if (esp_wifi_get_config(
            WIFI_IF_STA,
            &config
        ) != ESP_OK ||
        config.sta.ssid[0] == '\0') {
        return std::nullopt;
    }

    const auto* begin =
        reinterpret_cast<const char*>(
            config.sta.ssid
        );

    const auto length =
        strnlen(begin, sizeof(config.sta.ssid));

    return std::string(begin, length);
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

    if (event_id == WIFI_EVENT_STA_START) {
        self->started_.store(true);
    } else if (
        event_id == WIFI_EVENT_STA_STOP
    ) {
        self->started_.store(false);
        self->connected_.store(false);
    } else if (
        event_id == WIFI_EVENT_STA_CONNECTED
    ) {
        self->connected_.store(true);
    } else if (
        event_id == WIFI_EVENT_STA_DISCONNECTED
    ) {
        self->connected_.store(false);
    }
}

} // namespace enku::platform::esp_idf
