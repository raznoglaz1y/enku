#pragma once

#include "../core/app_state.hpp"
#include "../core/library.hpp"

namespace enku {

class BookDetailsRenderer {
public:
    virtual ~BookDetailsRenderer() = default;

    virtual bool renderBookDetails(
        const AppState& app_state,
        const BookRecord& book
    ) = 0;
};

} // namespace enku
