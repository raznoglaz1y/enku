#include <cstdint>
#include <string>
#include <vector>

#include "esp_log.h"
#include "esp_system.h"

#include "enku/platform/esp_idf/board.hpp"
#include "enku/platform/esp_idf/esp_idf_file_store.hpp"
#include "enku/platform/esp_idf/esp_idf_sd_card.hpp"

namespace {

constexpr const char* kTag = "ENKU";

bool storageSmokeTest(
    enku::platform::esp_idf::EspIdfFileStore& files
) {
    const std::string logical_path =
        "/system/tmp/platform-smoke.txt";
    const std::string payload =
        "ENKU ESP-IDF storage smoke test\n";

    if (files.write(logical_path, payload) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke write failed");
        return false;
    }

    std::string read_back;
    if (files.read(logical_path, read_back) !=
        enku::BookFileStatus::Ok ||
        read_back != payload) {
        ESP_LOGE(kTag, "Smoke read/verify failed");
        return false;
    }

    std::vector<std::string> tmp_files;
    if (files.list("/system/tmp", tmp_files) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke list failed");
        return false;
    }

    bool found = false;
    for (const auto& path : tmp_files) {
        if (path == logical_path) {
            found = true;
            break;
        }
    }

    if (!found) {
        ESP_LOGE(kTag, "Smoke file missing from list");
        return false;
    }

    if (files.remove(logical_path) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke remove failed");
        return false;
    }

    ESP_LOGI(kTag, "Storage smoke test passed");
    return true;
}

} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(
        kTag,
        "ENKU platform bring-up: ESP-IDF / ESP32-S3"
    );

    ESP_LOGI(
        kTag,
        "Free heap at boot: %lu bytes",
        static_cast<unsigned long>(
            esp_get_free_heap_size()
        )
    );

    enku::platform::esp_idf::EspIdfSdCard sd_card;
    const auto mount_status = sd_card.mount();

    if (mount_status !=
        enku::platform::esp_idf::SdMountStatus::Ok) {
        ESP_LOGE(
            kTag,
            "TF initialization failed; bring-up stopped"
        );
        return;
    }

    enku::platform::esp_idf::EspIdfFileStore files(
        enku::platform::esp_idf::board::kSdMountPoint
    );

    if (!storageSmokeTest(files)) {
        ESP_LOGE(
            kTag,
            "Platform storage adapter verification failed"
        );
        return;
    }

    ESP_LOGI(
        kTag,
        "ENKU platform storage bring-up complete"
    );
}
