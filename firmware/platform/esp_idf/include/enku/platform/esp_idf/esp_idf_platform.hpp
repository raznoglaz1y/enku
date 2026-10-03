#pragma once

#include <cstdint>

#include "enku/core/board_profile.hpp"
#include "enku/render/owned_mono_framebuffer.hpp"

#include "epaper_refresh_service.hpp"
#include "esp_idf_buttons.hpp"
#include "esp_idf_epaper.hpp"
#include "esp_idf_file_store.hpp"
#include "esp_idf_power_service.hpp"
#include "esp_idf_network_service.hpp"
#include "esp_idf_sd_card.hpp"

namespace enku::platform::esp_idf {

enum class PlatformInitStatus : std::uint8_t {
    Ok,
    SdMountFailed,
    SdDirectorySetupFailed,
    DisplayInitFailed,
    PowerInitFailed,
    NetworkInitFailed,
    ButtonsInitFailed,
};

class EspIdfPlatform {
public:
    EspIdfPlatform();

    PlatformInitStatus begin();

    EspIdfSdCard& sdCard();
    EspIdfFilesystem& filesystem();
    EspIdfStateFileStore& stateFiles();
    EspIdfBookFileStore& bookFiles();

    EspIdfEpaper& display();
    OwnedMonoFramebuffer& framebuffer();
    EpaperRefreshService& refresh();

    EspIdfPowerService& power();
    EspIdfNetworkService& network();
    EspIdfButtons& buttons();

    bool powerAvailable() const;
    bool networkAvailable() const;
    const BoardProfile& boardProfile() const;

private:
    EspIdfSdCard sd_card_;
    EspIdfFilesystem filesystem_;
    EspIdfStateFileStore state_files_;
    EspIdfBookFileStore book_files_;

    EspIdfEpaper display_;
    OwnedMonoFramebuffer framebuffer_;
    EpaperRefreshService refresh_;

    EspIdfPowerService power_;
    EspIdfNetworkService network_;
    EspIdfButtons buttons_;
    bool power_available_{false};
    bool network_available_{false};
};

} // namespace enku::platform::esp_idf
