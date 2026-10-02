#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../core/refresh.hpp"
#include "../reader/reader_session.hpp"
#include "../services/services.hpp"

namespace enku {

enum class ReaderRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    BeginningOfBook,
    EndOfBook,
    LayoutFailed,
    RefreshRejected,
};

class ReaderRuntimeController {
public:
    ReaderRuntimeController(
        AppState& app_state,
        ReaderSession& session,
        RefreshService& refresh
    );

    ReaderRuntimeResult handle(const PageNextRequested&);
    ReaderRuntimeResult handle(const PagePreviousRequested&);

    std::uint32_t refreshGeneration() const;

private:
    AppState& app_state_;
    ReaderSession& session_;
    RefreshService& refresh_;
    std::uint32_t refresh_generation_{0};

    ReaderRuntimeResult applySessionResult(
        ReaderSessionStatus status
    );

    ReaderRuntimeResult commitVisiblePage();
};

} // namespace enku
