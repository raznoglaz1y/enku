#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../core/library.hpp"
#include "../services/services.hpp"
#include "../storage/staged_book_import_service.hpp"
#include "reader_runtime.hpp"

namespace enku {

enum class LibraryRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Empty,
    QueryFailed,
    RefreshRejected,
    OpenFailed,
    ImportDuplicate,
    ImportFailed,
};

class LibraryRuntimeController {
public:
    LibraryRuntimeController(
        AppState& app_state,
        LibraryService& library,
        ReaderRuntimeController& reader,
        StagedBookImportService& importer,
        RefreshService& refresh
    );

    LibraryRuntimeResult handle(const LibraryRefreshRequested&);
    LibraryRuntimeResult handle(const LibraryFilterChanged&);
    LibraryRuntimeResult handle(const LibrarySortChanged&);
    LibraryRuntimeResult handle(const LibrarySearchChanged&);
    LibraryRuntimeResult handle(const LibraryFocusNextRequested&);
    LibraryRuntimeResult handle(const LibraryFocusPreviousRequested&);
    LibraryRuntimeResult handle(const OpenFocusedBookRequested&);
    LibraryRuntimeResult handle(const ImportRequested&);

    const LibraryPage& page() const;
    StagedImportStatus lastImportStatus() const;
    std::uint32_t refreshGeneration() const;

private:
    AppState& app_state_;
    LibraryService& library_;
    ReaderRuntimeController& reader_;
    StagedBookImportService& importer_;
    RefreshService& refresh_;

    LibraryPage page_;
    StagedImportStatus last_import_status_{
        StagedImportStatus::StageReadFailed
    };
    std::uint32_t refresh_generation_{0};

    LibraryQuery queryFromState() const;
    LibraryRuntimeResult reload(
        RefreshReason reason = RefreshReason::ScreenChanged
    );
    LibraryRuntimeResult submitRefresh(
        RefreshReason reason,
        RefreshClass refresh_class
    );
    void normalizeFocus();
    std::optional<std::size_t> focusedIndex() const;
};

} // namespace enku
