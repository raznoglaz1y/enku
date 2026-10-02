#pragma once

#include "../core/app_state.hpp"
#include "../core/reader_overlay.hpp"

namespace enku {

class ReaderOverlayRenderer {
public:
    virtual ~ReaderOverlayRenderer() = default;

    virtual bool renderReaderOverlay(
        const AppState& app_state
    ) = 0;
};

} // namespace enku
