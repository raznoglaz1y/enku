#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../render/book_finished_renderer.hpp"
#include "../services/services.hpp"
#include "../storage/cbor_reader_checkpoint.hpp"
#include "library_runtime.hpp"
#include "reader_runtime.hpp"

namespace enku {

enum class BookFinishedRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class BookFinishedRuntime {
public:
    BookFinishedRuntime(
        AppState& app_state,
        LibraryService& library,
        LibraryRuntimeController& library_runtime,
        ReaderRuntimeController& reader,
        CborReaderCheckpointService& checkpoints,
        BookFinishedRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    BookFinishedRuntimeResult openFromReader();
    BookFinishedRuntimeResult handle(
        LogicalAction action
    );

private:
    AppState& app_state_;
    LibraryService& library_;
    LibraryRuntimeController& library_runtime_;
    ReaderRuntimeController& reader_;
    CborReaderCheckpointService& checkpoints_;
    BookFinishedRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    BookFinishedRuntimeResult render();
    BookFinishedRuntimeResult backToLibrary();
    BookFinishedRuntimeResult returnToLastPage();
    BookFinishedRuntimeResult restartReading();
};

} // namespace enku
