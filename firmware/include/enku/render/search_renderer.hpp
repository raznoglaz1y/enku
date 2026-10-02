#pragma once

#include "../core/app_state.hpp"

namespace enku {

class SearchRenderer {
public:
    virtual ~SearchRenderer() = default;

    virtual bool renderSearch(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
