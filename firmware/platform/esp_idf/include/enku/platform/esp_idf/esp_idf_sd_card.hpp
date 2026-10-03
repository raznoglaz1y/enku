#pragma once

#include <cstdint>

#include "sdmmc_cmd.h"

namespace enku::platform::esp_idf {

enum class SdMountStatus : std::uint8_t {
    Ok,
    MountFailed,
    DirectorySetupFailed,
};

class EspIdfSdCard {
public:
    EspIdfSdCard() = default;
    ~EspIdfSdCard();

    SdMountStatus mount();
    void unmount();

    bool mounted() const;
    bool healthy() const;
    sdmmc_card_t* card() const;

private:
    sdmmc_card_t* card_{nullptr};
    bool mounted_{false};

    bool ensureDirectory(const char* path) const;
};

} // namespace enku::platform::esp_idf
