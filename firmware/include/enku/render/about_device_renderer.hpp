#pragma once

#include "../core/app_state.hpp"

namespace enku {

class AboutDeviceRenderer {
public:
    virtual ~AboutDeviceRenderer() = default;

    virtual bool renderAboutDevice(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
