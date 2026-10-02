#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../core/library.hpp"
#include "../services/services.hpp"
#include "../render/library_page_renderer.hpp"
#include "../storage/staged_book_import_service.hpp"
#include "../storage/book_delete_service.hpp"
#include "reader_runtime.hpp"
#include "settings_runtime.hpp"

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
    DeleteFailed,
    SettingsSaveFailed,
    RenderFailed,
};

class LibraryRuntimeController {
public:
    LibraryRuntimeController(
        AppState& app_state,
        LibraryService& library,
        ReaderRuntimeController& reader,
        StagedBookImportService& importer,
        BookDeleteService& deleter,
        SettingsRuntimeController& settings,
        RefreshService& refresh,
        LibraryPageRenderer* page_renderer = nullptr
    );

    LibraryRuntimeResult handle(const LibraryRefreshRequested&);
    LibraryRuntimeResult handle(const LibraryFilterChanged&);
    LibraryRuntimeResult handle(const LibrarySortChanged&);
    LibraryRuntimeResult handle(const LibraryViewChanged&);
    LibraryRuntimeResult handle(const LibrarySearchChanged&);
    LibraryRuntimeResult handle(const LibraryFocusNextRequested&);
    LibraryRuntimeResult handle(const LibraryFocusPreviousRequested&);
    LibraryRuntimeResult handle(const OpenFocusedBookRequested&);
    LibraryRuntimeResult handle(const DeleteFocusedBookRequested&);
    LibraryRuntimeResult handle(const ImportRequested&);

    LibraryRuntimeResult redraw(
        RefreshReason reason = RefreshReason::OverlayChanged,
        RefreshClass refresh_class = RefreshClass::Full
    );

    const LibraryPage& page() const;
    StagedImportStatus lastImportStatus() const;
    std::uint32_t refreshGeneration() const;

private:
    AppState& app_state_;
    LibraryService& library_;
    ReaderRuntimeController& reader_;
    StagedBookImportService& importer_;
    BookDeleteService& deleter_;
    SettingsRuntimeController& settings_;
    RefreshService& refresh_;
    LibraryPageRenderer* page_renderer_{nullptr};

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
