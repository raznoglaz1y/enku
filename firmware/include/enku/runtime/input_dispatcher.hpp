#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/input.hpp"
#include "input_runtime.hpp"
#include "library_runtime.hpp"
#include "library_search_runtime.hpp"
#include "power_off.hpp"
#include "reader_overlay_runtime.hpp"
#include "reader_runtime.hpp"
#include "search_runtime.hpp"
#include "sleep_wake.hpp"

namespace enku {

enum class InputDispatchResult : std::uint8_t {
    Ignored,
    Applied,
    Unhandled,
    Failed,
};

class InputDispatcher {
public:
    InputDispatcher(
        AppState& app_state,
        LibraryRuntimeController& library,
        ReaderRuntimeController& reader,
        SleepWakeCoordinator& sleep_wake,
        PowerOffCoordinator& power_off,
        ReaderOverlayRuntime* reader_overlay = nullptr,
        SearchRuntime* search = nullptr,
        LibrarySearchRuntime* library_search = nullptr
    );

    InputDispatchResult handle(
        const PhysicalInputEvent& input
    );

private:
    AppState& app_state_;
    LibraryRuntimeController& library_;
    ReaderRuntimeController& reader_;
    SleepWakeCoordinator& sleep_wake_;
    PowerOffCoordinator& power_off_;
    ReaderOverlayRuntime* reader_overlay_{nullptr};
    SearchRuntime* search_{nullptr};
    LibrarySearchRuntime* library_search_{nullptr};
};

} // namespace enku
