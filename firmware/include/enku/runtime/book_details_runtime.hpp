#pragma once

#include <cstdint>
#include <optional>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../render/book_details_renderer.hpp"
#include "../services/services.hpp"
#include "../storage/cbor_reader_checkpoint.hpp"
#include "../storage/book_delete_service.hpp"
#include "library_runtime.hpp"
#include "reader_runtime.hpp"

namespace enku {

enum class BookDetailsRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class BookDetailsRuntime {
public:
    BookDetailsRuntime(
        AppState& app_state,
        LibraryService& library,
        LibraryRuntimeController& library_runtime,
        ReaderRuntimeController& reader,
        CborReaderCheckpointService& checkpoints,
        BookDeleteService& delete_service,
        BookDetailsRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    BookDetailsRuntimeResult handle(
        const OpenFocusedBookDetailsRequested&
    );

    BookDetailsRuntimeResult handle(
        LogicalAction action
    );

    std::optional<BookRecord> currentBook() const;
    const char* primaryActionLabel() const;

private:
    AppState& app_state_;
    LibraryService& library_;
    LibraryRuntimeController& library_runtime_;
    ReaderRuntimeController& reader_;
    CborReaderCheckpointService& checkpoints_;
    BookDeleteService& delete_service_;
    BookDetailsRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    std::uint32_t refresh_generation_{0};

    BookDetailsRuntimeResult openFocused();
    BookDetailsRuntimeResult close();
    BookDetailsRuntimeResult render();
    BookDetailsRuntimeResult activateFocused();
    BookDetailsRuntimeResult restartReading();
    BookDetailsRuntimeResult removeBook();
    BookDetailsRuntimeResult moveFocus(int direction);
    bool hasRestartAction(const BookRecord& book) const;
};

} // namespace enku
