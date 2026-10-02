#pragma once

#include "../core/app_state.hpp"

namespace enku {

class DisplaySettingsRenderer {
public:
    virtual ~DisplaySettingsRenderer() = default;

    virtual bool renderDisplaySettings(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
