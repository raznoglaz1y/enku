#pragma once

#include "../core/app_state.hpp"

namespace enku {

class LocaleSettingsRenderer {
public:
    virtual ~LocaleSettingsRenderer() = default;

    virtual bool renderLocaleSettings(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
