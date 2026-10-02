#pragma once

#include "../core/app_state.hpp"

namespace enku {

class ReadingSettingsRenderer {
public:
    virtual ~ReadingSettingsRenderer() = default;

    virtual bool renderReadingSettings(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
