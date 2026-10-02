#include "enku/platform/esp_idf/esp_idf_sd_card.hpp"

#include <cerrno>
#include <sys/stat.h>

#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "enku/platform/esp_idf/board.hpp"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_SD";

} // namespace

EspIdfSdCard::~EspIdfSdCard() {
    unmount();
}

bool EspIdfSdCard::ensureDirectory(
    const char* path
) const {
    if (::mkdir(path, 0775) == 0) {
        return true;
    }

    return errno == EEXIST;
}

SdMountStatus EspIdfSdCard::mount() {
    if (mounted_) {
        return SdMountStatus::Ok;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config =
        SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 4;
    slot_config.clk = board::kSdClk;
    slot_config.cmd = board::kSdCmd;
    slot_config.d0 = board::kSdD0;
    slot_config.d1 = board::kSdD1;
    slot_config.d2 = board::kSdD2;
    slot_config.d3 = board::kSdD3;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 8;
    mount_config.allocation_unit_size =
        16 * 1024;

    const esp_err_t err =
        esp_vfs_fat_sdmmc_mount(
            board::kSdMountPoint,
            &host,
            &slot_config,
            &mount_config,
            &card_
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            kTag,
            "TF mount failed: %s",
            esp_err_to_name(err)
        );
        card_ = nullptr;
        return SdMountStatus::MountFailed;
    }

    mounted_ = true;
    sdmmc_card_print_info(stdout, card_);

    const char* directories[] = {
        "/sdcard/books",
        "/sdcard/system",
        "/sdcard/system/state",
        "/sdcard/system/covers",
        "/sdcard/system/fonts",
        "/sdcard/system/tmp",
    };

    for (const auto* path : directories) {
        if (!ensureDirectory(path)) {
            ESP_LOGE(
                kTag,
                "Failed to create %s (errno=%d)",
                path,
                errno
            );
            return SdMountStatus::DirectorySetupFailed;
        }
    }

    ESP_LOGI(kTag, "TF mounted at %s", board::kSdMountPoint);
    return SdMountStatus::Ok;
}

void EspIdfSdCard::unmount() {
    if (!mounted_) {
        return;
    }

    esp_vfs_fat_sdcard_unmount(
        board::kSdMountPoint,
        card_
    );

    card_ = nullptr;
    mounted_ = false;
}

bool EspIdfSdCard::mounted() const {
    return mounted_;
}

sdmmc_card_t* EspIdfSdCard::card() const {
    return card_;
}

} // namespace enku::platform::esp_idf
