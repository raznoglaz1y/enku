#pragma once

#include "../core/app_state.hpp"

namespace enku {

class PowerOffConfirmRenderer {
public:
    virtual ~PowerOffConfirmRenderer() = default;

    virtual bool renderPowerOffConfirm(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
