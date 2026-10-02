#pragma once

#include <cstdint>

#include "../core/input.hpp"
#include "../core/events.hpp"
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
        const OpenLibrarySearchRequested&
    );
    LibrarySearchRuntimeResult handle(
        LogicalAction action
    );
    LibrarySearchRuntimeResult submit();

private:
    AppState& app_state_;
    LibraryRuntimeController& library_;
    KeyboardRuntime keyboard_;
    bool origin_valid_{false};
    std::uint32_t origin_offset_{0};
    std::optional<BookId> origin_focused_book_;

    LibrarySearchRuntimeResult applyQuery();
    LibrarySearchRuntimeResult cancel();
};

} // namespace enku
