#pragma once

#include <cstdint>

#include "../core/input.hpp"
#include "keyboard_runtime.hpp"
#include "library_runtime.hpp"

namespace enku {

enum class LibrarySearchRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class LibrarySearchRuntime {
public:
    LibrarySearchRuntime(
        AppState& app_state,
        LibraryRuntimeController& library
    );

    LibrarySearchRuntimeResult open();
    LibrarySearchRuntimeResult handle(
        LogicalAction action
    );
    LibrarySearchRuntimeResult submit();

private:
    AppState& app_state_;
    LibraryRuntimeController& library_;
    KeyboardRuntime keyboard_;

    LibrarySearchRuntimeResult applyQuery();
    LibrarySearchRuntimeResult cancel();
};

} // namespace enku
