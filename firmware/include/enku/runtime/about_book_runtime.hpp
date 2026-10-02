#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/about_book_renderer.hpp"
#include "../services/services.hpp"
#include "application_reader_runtime.hpp"

namespace enku {

enum class AboutBookRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class AboutBookRuntime {
public:
    AboutBookRuntime(
        AppState& app_state,
        LibraryService& library,
        ApplicationReaderRuntime& reader,
        AboutBookRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    AboutBookRuntimeResult openFromReader();

    AboutBookRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    LibraryService& library_;
    ApplicationReaderRuntime& reader_;
    AboutBookRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    AboutBookRuntimeResult render();
    AboutBookRuntimeResult close();
};

} // namespace enku
