#pragma once

#include "../core/app_state.hpp"

namespace enku {

class SettingsScreenRenderer {
public:
    virtual ~SettingsScreenRenderer() = default;

    virtual bool renderSettingsScreen(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
