#include "enku/platform/esp_idf/esp_idf_internal_state_storage.hpp"

#include <cstddef>

#include "esp_log.h"
#include "esp_spiffs.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_STATE";

} // namespace

EspIdfInternalStateStorage::~EspIdfInternalStateStorage() {
    if (mounted_) {
        esp_vfs_spiffs_unregister(
            kPartitionLabel
        );
        mounted_ = false;
    }
}

InternalStateMountStatus
EspIdfInternalStateStorage::begin() {
    if (mounted_) {
        return InternalStateMountStatus::Ok;
    }

    esp_vfs_spiffs_conf_t config = {};
    config.base_path = kMountPoint;
    config.partition_label = kPartitionLabel;
    config.max_files = 16;
    config.format_if_mount_failed = true;

    const auto result =
        esp_vfs_spiffs_register(&config);

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "Internal state mount failed: %s",
            esp_err_to_name(result)
        );
        return InternalStateMountStatus::MountFailed;
    }

    std::size_t total = 0;
    std::size_t used = 0;

    const auto info_result =
        esp_spiffs_info(
            kPartitionLabel,
            &total,
            &used
        );

    if (info_result != ESP_OK) {
        ESP_LOGW(
            kTag,
            "Internal state mounted but usage query failed: %s",
            esp_err_to_name(info_result)
        );
    } else {
        ESP_LOGI(
            kTag,
            "Internal state ready at %s: %u/%u bytes used",
            kMountPoint,
            static_cast<unsigned>(used),
            static_cast<unsigned>(total)
        );
    }

    mounted_ = true;
    return InternalStateMountStatus::Ok;
}

bool EspIdfInternalStateStorage::mounted() const {
    return mounted_;
}

} // namespace enku::platform::esp_idf
