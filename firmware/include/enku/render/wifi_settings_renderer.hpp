#pragma once

#include "../core/app_state.hpp"

namespace enku {

class WiFiSettingsRenderer {
public:
    virtual ~WiFiSettingsRenderer() = default;

    virtual bool renderWiFiSettings(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
