#include "enku/platform/esp_idf/esp_idf_platform.hpp"

#include "enku/platform/esp_idf/board.hpp"
#include "enku/platform/esp_idf/board_profiles.hpp"

namespace enku::platform::esp_idf {

EspIdfPlatform::EspIdfPlatform()
    : filesystem_(board::kSdMountPoint),
      state_files_(filesystem_),
      book_files_(filesystem_),
      framebuffer_(
          EspIdfEpaper::kWidth,
          EspIdfEpaper::kHeight
      ),
      refresh_(display_, framebuffer_) {}

PlatformInitStatus EspIdfPlatform::begin() {
    if (sd_card_.mount() != SdMountStatus::Ok) {
        return PlatformInitStatus::SdMountFailed;
    }

    if (!refresh_.begin()) {
        return PlatformInitStatus::DisplayInitFailed;
    }

    if (!power_.begin()) {
        return PlatformInitStatus::PowerInitFailed;
    }

    if (!network_.begin()) {
        return PlatformInitStatus::NetworkInitFailed;
    }

    if (!buttons_.begin()) {
        return PlatformInitStatus::ButtonsInitFailed;
    }

    return PlatformInitStatus::Ok;
}

EspIdfSdCard& EspIdfPlatform::sdCard() {
    return sd_card_;
}

EspIdfFilesystem& EspIdfPlatform::filesystem() {
    return filesystem_;
}

EspIdfStateFileStore& EspIdfPlatform::stateFiles() {
    return state_files_;
}

EspIdfBookFileStore& EspIdfPlatform::bookFiles() {
    return book_files_;
}

EspIdfEpaper& EspIdfPlatform::display() {
    return display_;
}

OwnedMonoFramebuffer& EspIdfPlatform::framebuffer() {
    return framebuffer_;
}

EpaperRefreshService& EspIdfPlatform::refresh() {
    return refresh_;
}

EspIdfPowerService& EspIdfPlatform::power() {
    return power_;
}

EspIdfNetworkService& EspIdfPlatform::network() {
    return network_;
}

EspIdfButtons& EspIdfPlatform::buttons() {
    return buttons_;
}

const BoardProfile& EspIdfPlatform::boardProfile() const {
    return board_profiles::kWaveshareEsp32S3Epaper397;
}

} // namespace enku::platform::esp_idf
