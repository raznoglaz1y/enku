#pragma once

#include "../core/app_state.hpp"
#include "../core/library.hpp"

namespace enku {

class AboutBookRenderer {
public:
    virtual ~AboutBookRenderer() = default;

    virtual bool renderAboutBook(
        const AppState& app_state,
        const BookRecord& book
    ) = 0;
};

} // namespace enku
