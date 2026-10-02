#include "enku/platform/esp_idf/esp_idf_network_service.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_WIFI";

} // namespace

EspIdfNetworkService::~EspIdfNetworkService() {
    if (reconnect_timer_ != nullptr) {
        esp_timer_stop(reconnect_timer_);
        esp_timer_delete(reconnect_timer_);
        reconnect_timer_ = nullptr;
    }

    if (ip_handler_ != nullptr) {
        esp_event_handler_instance_unregister(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            ip_handler_
        );
        ip_handler_ = nullptr;
    }

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

    result =
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &EspIdfNetworkService::handleIpEvent,
            this,
            &ip_handler_
        );

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "IP event registration failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    esp_timer_create_args_t reconnect_timer_args = {};
    reconnect_timer_args.callback =
        &EspIdfNetworkService::reconnectTimerCallback;
    reconnect_timer_args.arg = this;
    reconnect_timer_args.name = "enku-wifi-retry";

    if (esp_timer_create(
            &reconnect_timer_args,
            &reconnect_timer_
        ) != ESP_OK) {
        ESP_LOGE(kTag, "Wi-Fi reconnect timer init failed");
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

NetworkLinkState
EspIdfNetworkService::connectionState() const {
    return link_state_.load();
}

void EspIdfNetworkService::disconnect() {
    manual_disconnect_.store(true);
    resetReconnect();
    connected_.store(false);
    link_state_.store(
        NetworkLinkState::Disconnected
    );

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

    active_policy_.store(policy);
    resetReconnect();

    switch (policy) {
        case WiFiPolicy::Off:
            disconnect();

            if (started_.load() &&
                esp_wifi_stop() != ESP_OK) {
                return NetworkPolicyStatus::DriverError;
            }

            started_.store(false);
            link_state_.store(
                NetworkLinkState::Disconnected
            );
            return NetworkPolicyStatus::Ok;

        case WiFiPolicy::Manual:
            if (!ensureStarted()) {
                return NetworkPolicyStatus::DriverError;
            }

            disconnect();
            link_state_.store(
                NetworkLinkState::Disconnected
            );
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

            manual_disconnect_.store(false);
            link_state_.store(
                NetworkLinkState::Connecting
            );

            if (esp_wifi_connect() != ESP_OK) {
                link_state_.store(
                    NetworkLinkState::Failed
                );
                return NetworkPolicyStatus::DriverError;
            }

            return NetworkPolicyStatus::Ok;
        }

        default:
            return NetworkPolicyStatus::DriverError;
    }
}

NetworkPolicyStatus EspIdfNetworkService::scanNetworks(
    std::vector<WiFiNetworkInfo>& networks
) {
    networks.clear();

    if (!initialized_ || !ensureStarted()) {
        return NetworkPolicyStatus::DriverError;
    }

    wifi_scan_config_t scan_config = {};

    const auto scan_result =
        esp_wifi_scan_start(
            &scan_config,
            true
        );

    if (scan_result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "Wi-Fi scan failed: %s",
            esp_err_to_name(scan_result)
        );
        return NetworkPolicyStatus::DriverError;
    }

    std::uint16_t count = 0;
    if (esp_wifi_scan_get_ap_num(&count) != ESP_OK) {
        return NetworkPolicyStatus::DriverError;
    }

    if (count == 0) {
        return NetworkPolicyStatus::Ok;
    }

    std::vector<wifi_ap_record_t> records(count);
    auto requested = count;

    if (esp_wifi_scan_get_ap_records(
            &requested,
            records.data()
        ) != ESP_OK) {
        return NetworkPolicyStatus::DriverError;
    }

    records.resize(requested);

    for (const auto& record : records) {
        const auto* raw_ssid =
            reinterpret_cast<const char*>(
                record.ssid
            );

        const auto length =
            strnlen(
                raw_ssid,
                sizeof(record.ssid)
            );

        if (length == 0U) {
            continue;
        }

        const std::string ssid(
            raw_ssid,
            length
        );

        const auto duplicate =
            std::find_if(
                networks.begin(),
                networks.end(),
                [&](const WiFiNetworkInfo& item) {
                    return item.ssid == ssid;
                }
            );

        if (duplicate != networks.end()) {
            if (record.rssi > duplicate->rssi) {
                duplicate->rssi = record.rssi;
            }
            continue;
        }

        WiFiNetworkInfo info;
        info.ssid = ssid;
        info.rssi = record.rssi;
        info.secured =
            record.authmode != WIFI_AUTH_OPEN;

        networks.push_back(
            std::move(info)
        );
    }

    std::sort(
        networks.begin(),
        networks.end(),
        [](const WiFiNetworkInfo& lhs,
           const WiFiNetworkInfo& rhs) {
            return lhs.rssi > rhs.rssi;
        }
    );

    ESP_LOGI(
        kTag,
        "Wi-Fi scan completed: %u network(s)",
        static_cast<unsigned>(networks.size())
    );

    return NetworkPolicyStatus::Ok;
}

NetworkPolicyStatus EspIdfNetworkService::connectToNetwork(
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

    // Preserve the currently active trusted configuration. A RAM-backed
    // esp_wifi_set_config() still replaces the live STA configuration, so
    // a failed transient attempt must restore this snapshot explicitly.
    wifi_config_t previous_config = {};
    if (esp_wifi_get_config(
            WIFI_IF_STA,
            &previous_config
        ) != ESP_OK) {
        return NetworkPolicyStatus::DriverError;
    }

    disconnect();

    // Use RAM storage so credentials used for this attempt cannot replace
    // the trusted network before the connection has actually succeeded.
    if (esp_wifi_set_storage(
            WIFI_STORAGE_RAM
        ) != ESP_OK) {
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

    const auto config_result =
        esp_wifi_set_config(
            WIFI_IF_STA,
            &config
        );

    // Subsequent setTrustedNetwork() calls are durable again. Switching
    // storage does not itself persist the transient config.
    const auto storage_result =
        esp_wifi_set_storage(
            WIFI_STORAGE_FLASH
        );

    if (config_result != ESP_OK ||
        storage_result != ESP_OK) {
        // Best-effort restore when the transient setup only partially
        // succeeded. Writing the previous trusted config to FLASH is safe:
        // it is the same durable configuration we started with.
        if (storage_result == ESP_OK) {
            esp_wifi_set_config(
                WIFI_IF_STA,
                &previous_config
            );
        }
        return NetworkPolicyStatus::DriverError;
    }

    connected_.store(false);
    manual_disconnect_.store(false);
    transient_connect_.store(true);
    link_state_.store(
        NetworkLinkState::Connecting
    );

    const auto connect_result =
        esp_wifi_connect();

    if (connect_result != ESP_OK) {
        transient_connect_.store(false);
        link_state_.store(
            NetworkLinkState::Failed
        );
        esp_wifi_set_config(
            WIFI_IF_STA,
            &previous_config
        );
        return NetworkPolicyStatus::DriverError;
    }

    constexpr int kConnectPolls = 100;
    for (int i = 0; i < kConnectPolls; ++i) {
        if (connected_.load()) {
            transient_connect_.store(false);
            ESP_LOGI(
                kTag,
                "Wi-Fi connected to %.*s",
                static_cast<int>(ssid.size()),
                ssid.data()
            );
            return NetworkPolicyStatus::Ok;
        }

        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }

    disconnect();
    transient_connect_.store(false);

    const auto restore_result =
        esp_wifi_set_config(
            WIFI_IF_STA,
            &previous_config
        );

    if (restore_result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "Failed to restore trusted Wi-Fi config: %s",
            esp_err_to_name(restore_result)
        );
        return NetworkPolicyStatus::DriverError;
    }

    link_state_.store(
        NetworkLinkState::Failed
    );

    ESP_LOGW(
        kTag,
        "Wi-Fi connection failed for %.*s; trusted config restored",
        static_cast<int>(ssid.size()),
        ssid.data()
    );

    return NetworkPolicyStatus::ConnectionFailed;
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
        self->link_state_.store(
            NetworkLinkState::Disconnected
        );
        self->resetReconnect();
    } else if (
        event_id == WIFI_EVENT_STA_CONNECTED
    ) {
        // Association alone is not enough. connected() becomes true only
        // after IP_EVENT_STA_GOT_IP.
        self->connected_.store(false);
        self->link_state_.store(
            NetworkLinkState::Connecting
        );
    } else if (
        event_id == WIFI_EVENT_STA_DISCONNECTED
    ) {
        self->connected_.store(false);

        const bool manual =
            self->manual_disconnect_.exchange(false);

        if (!manual &&
            !self->transient_connect_.load() &&
            self->active_policy_.load() ==
                WiFiPolicy::AutoConnectTrusted) {
            self->scheduleReconnect();
        } else {
            self->link_state_.store(
                NetworkLinkState::Disconnected
            );
        }
    }
}

void EspIdfNetworkService::handleIpEvent(
    void* arg,
    esp_event_base_t,
    std::int32_t event_id,
    void*
) {
    auto* self =
        static_cast<EspIdfNetworkService*>(arg);

    if (self == nullptr ||
        event_id != IP_EVENT_STA_GOT_IP) {
        return;
    }

    self->connected_.store(true);
    self->link_state_.store(
        NetworkLinkState::Online
    );
    self->manual_disconnect_.store(false);
    self->resetReconnect();

    ESP_LOGI(
        kTag,
        "Wi-Fi link is online and has an IP address"
    );
}

void EspIdfNetworkService::resetReconnect() {
    reconnect_attempt_.store(0);

    if (reconnect_timer_ != nullptr &&
        esp_timer_is_active(reconnect_timer_)) {
        esp_timer_stop(reconnect_timer_);
    }
}

void EspIdfNetworkService::scheduleReconnect() {
    constexpr std::uint8_t kMaxReconnectAttempts = 3;

    if (!initialized_ ||
        !started_.load() ||
        active_policy_.load() !=
            WiFiPolicy::AutoConnectTrusted ||
        reconnect_timer_ == nullptr) {
        return;
    }

    const auto attempt =
        reconnect_attempt_.load();

    if (attempt >= kMaxReconnectAttempts) {
        link_state_.store(
            NetworkLinkState::Failed
        );
        ESP_LOGW(
            kTag,
            "Wi-Fi reconnect limit reached"
        );
        return;
    }

    static constexpr std::uint64_t kRetryDelayUs[] = {
        1'000'000ULL,
        2'000'000ULL,
        4'000'000ULL,
    };

    reconnect_attempt_.store(
        static_cast<std::uint8_t>(attempt + 1U)
    );
    link_state_.store(
        NetworkLinkState::Connecting
    );

    if (esp_timer_is_active(reconnect_timer_)) {
        esp_timer_stop(reconnect_timer_);
    }

    const auto timer_result =
        esp_timer_start_once(
            reconnect_timer_,
            kRetryDelayUs[attempt]
        );

    if (timer_result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "Failed to schedule Wi-Fi reconnect: %s",
            esp_err_to_name(timer_result)
        );
        return;
    }

    ESP_LOGW(
        kTag,
        "Wi-Fi disconnected; retry %u/%u scheduled",
        static_cast<unsigned>(attempt + 1U),
        static_cast<unsigned>(kMaxReconnectAttempts)
    );
}

void EspIdfNetworkService::reconnectTimerCallback(
    void* arg
) {
    auto* self =
        static_cast<EspIdfNetworkService*>(arg);

    if (self == nullptr ||
        self->active_policy_.load() !=
            WiFiPolicy::AutoConnectTrusted ||
        !self->started_.load() ||
        self->connected_.load()) {
        return;
    }

    self->manual_disconnect_.store(false);

    const auto result =
        esp_wifi_connect();

    if (result != ESP_OK) {
        self->link_state_.store(
            NetworkLinkState::Failed
        );
        ESP_LOGW(
            kTag,
            "Wi-Fi reconnect call failed: %s",
            esp_err_to_name(result)
        );
        self->scheduleReconnect();
    }
}

} // namespace enku::platform::esp_idf
