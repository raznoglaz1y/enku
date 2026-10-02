#pragma once

#include <optional>

#include "../core/app_state.hpp"
#include "../core/input.hpp"

namespace enku {

class InputActionMapper {
public:
    static std::optional<LogicalAction> map(
        const AppState& app_state,
        const PhysicalInputEvent& input
    );
};

} // namespace enku
