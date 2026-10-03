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
    // Bring up the user-visible/control path before removable storage.
    // If the SD card is missing or unreadable, app_main can still render a
    // deterministic recovery screen instead of failing before the display
    // and controls exist.
    if (!refresh_.begin()) {
        return PlatformInitStatus::DisplayInitFailed;
    }

    if (!power_.begin()) {
        return PlatformInitStatus::PowerInitFailed;
    }

    // Networking is optional for an offline-first reader. Preserve the
    // failure state for UI/diagnostics, but do not block Library/Reading.
    network_available_ = network_.begin();

    if (!buttons_.begin()) {
        return PlatformInitStatus::ButtonsInitFailed;
    }

    const auto sd_status = sd_card_.mount();

    if (sd_status == SdMountStatus::MountFailed) {
        return PlatformInitStatus::SdMountFailed;
    }

    if (sd_status ==
        SdMountStatus::DirectorySetupFailed) {
        return PlatformInitStatus::SdDirectorySetupFailed;
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

bool EspIdfPlatform::networkAvailable() const {
    return network_available_;
}

const BoardProfile& EspIdfPlatform::boardProfile() const {
    return board_profiles::kWaveshareEsp32S3Epaper397;
}

} // namespace enku::platform::esp_idf
