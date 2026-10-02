#pragma once

#include "../core/app_state.hpp"
#include "../storage/book_delete_service.hpp"
#include "../storage/book_import_service.hpp"
#include "../storage/cbor_app_context_service.hpp"
#include "../storage/cbor_boot_loop_service.hpp"
#include "../storage/cbor_library_service.hpp"
#include "../storage/cbor_reader_checkpoint.hpp"
#include "../storage/cbor_settings_service.hpp"
#include "../storage/staged_book_import_service.hpp"
#include "../storage/stored_book_source_service.hpp"
#include "settings_runtime.hpp"
#include "storage_startup.hpp"

namespace enku {

class ApplicationStorageRuntime {
public:
    ApplicationStorageRuntime(
        StateFileStore& state_files,
        BookFileStore& book_files
    );

    AppState& appState();

    CborSettingsService& settingsStore();
    SettingsRuntimeController& settingsRuntime();

    CborLibraryService& library();
    BookImportService& importCore();
    StagedBookImportService& stagedImport();
    BookDeleteService& deleteService();

    CborReaderCheckpointService& checkpoints();
    CborAppContextService& appContext();
    CborBootLoopService& bootLoop();
    StoredBookSourceService& bookSource();

    StorageStartupCoordinator& storageStartup();

private:
    AppState app_state_;

    CborSettingsService settings_store_;
    SettingsRuntimeController settings_runtime_;

    CborLibraryService library_;
    BookImportService import_core_;
    StagedBookImportService staged_import_;

    CborReaderCheckpointService checkpoints_;
    CborAppContextService app_context_;
    CborBootLoopService boot_loop_;

    BookDeleteService delete_service_;
    StoredBookSourceService book_source_;

    StorageStartupCoordinator storage_startup_;
};

} // namespace enku
