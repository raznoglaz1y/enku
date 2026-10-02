#pragma once

#include "../core/app_state.hpp"
#include "../core/library.hpp"

namespace enku {

class LibraryPageRenderer {
public:
    virtual ~LibraryPageRenderer() = default;

    virtual bool renderLibrary(
        const AppState& app_state,
        const LibraryPage& page
    ) = 0;
};

} // namespace enku
