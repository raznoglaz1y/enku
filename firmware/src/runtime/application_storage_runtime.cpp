#include "enku/runtime/application_storage_runtime.hpp"

namespace enku {

ApplicationStorageRuntime::ApplicationStorageRuntime(
    StateFileStore& state_files,
    BookFileStore& book_files
)
    : settings_store_(state_files),
      settings_runtime_(
          app_state_,
          settings_store_
      ),
      library_(state_files),
      import_core_(library_),
      staged_import_(
          book_files,
          import_core_
      ),
      web_upload_(
          book_files,
          staged_import_
      ),
      checkpoints_(state_files),
      bookmarks_(state_files),
      app_context_(state_files),
      boot_loop_(state_files),
      delete_service_(
          library_,
          book_files,
          checkpoints_,
          app_context_,
          &bookmarks_
      ),
      book_source_(book_files),
      storage_startup_(
          app_state_,
          settings_runtime_,
          library_,
          book_files,
          import_core_
      ) {}

AppState& ApplicationStorageRuntime::appState() {
    return app_state_;
}

CborSettingsService&
ApplicationStorageRuntime::settingsStore() {
    return settings_store_;
}

SettingsRuntimeController&
ApplicationStorageRuntime::settingsRuntime() {
    return settings_runtime_;
}

CborLibraryService&
ApplicationStorageRuntime::library() {
    return library_;
}

BookImportService&
ApplicationStorageRuntime::importCore() {
    return import_core_;
}

StagedBookImportService&
ApplicationStorageRuntime::stagedImport() {
    return staged_import_;
}

WebUploadIngress&
ApplicationStorageRuntime::webUpload() {
    return web_upload_;
}

BookDeleteService&
ApplicationStorageRuntime::deleteService() {
    return delete_service_;
}

CborReaderCheckpointService&
ApplicationStorageRuntime::checkpoints() {
    return checkpoints_;
}

CborBookmarkService&
ApplicationStorageRuntime::bookmarks() {
    return bookmarks_;
}

CborAppContextService&
ApplicationStorageRuntime::appContext() {
    return app_context_;
}

CborBootLoopService&
ApplicationStorageRuntime::bootLoop() {
    return boot_loop_;
}

StoredBookSourceService&
ApplicationStorageRuntime::bookSource() {
    return book_source_;
}

StorageStartupCoordinator&
ApplicationStorageRuntime::storageStartup() {
    return storage_startup_;
}

} // namespace enku
