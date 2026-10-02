#pragma once

#include "../core/app_state.hpp"
#include "../core/library.hpp"

namespace enku {

class BookFinishedRenderer {
public:
    virtual ~BookFinishedRenderer() = default;

    virtual bool renderBookFinished(
        const AppState& app_state,
        const BookRecord& book
    ) = 0;
};

} // namespace enku
