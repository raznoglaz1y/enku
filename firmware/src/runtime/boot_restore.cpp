#include "enku/runtime/boot_restore.hpp"

namespace enku {

BootRestoreCoordinator::BootRestoreCoordinator(
    AppState& app_state,
    StorageStartupCoordinator& storage_startup,
    AppContextService& context,
    ReaderRuntimeController& reader_runtime,
    LibraryService& library
)
    : app_state_(app_state),
      storage_startup_(storage_startup),
      context_(context),
      reader_runtime_(reader_runtime),
      library_(library) {}

bool BootRestoreCoordinator::persistLibraryContext() {
    return context_.save(
        AppRestoreContext{
            Screen::Library,
            std::nullopt,
        }
    ) == PersistStatus::Ok;
}

void BootRestoreCoordinator::settleLibrary() {
    app_state_.screen = Screen::Library;
    app_state_.current_book.reset();
    app_state_.reading_position.reset();
    app_state_.reading_progress = 0.0F;
    app_state_.current_book_finished = false;
    app_state_.progress_dirty = false;
    app_state_.boot.mode = BootMode::Normal;
    app_state_.boot.stage = BootStage::Stable;
    app_state_.boot.boot_in_progress = false;
}

BootRestoreResult BootRestoreCoordinator::restoreLoadedContext(
    const StorageStartupResult& storage
) {
    app_state_.boot.stage = BootStage::Restore;
    app_state_.boot.boot_in_progress = true;

    AppRestoreContext restore;
    const auto context_status =
        context_.load(restore);

    if (context_status == PersistStatus::NotFound) {
        if (!persistLibraryContext()) {
            app_state_.boot.mode = BootMode::Recovery;
            app_state_.boot.stage = BootStage::RecoveryMode;
            app_state_.boot.boot_in_progress = false;
            app_state_.screen = Screen::ErrorRecovery;

            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }

        settleLibrary();
        return BootRestoreResult{
            BootRestoreStatus::LibraryReady,
            storage,
        };
    }

    if (context_status != PersistStatus::Ok) {
        if (!persistLibraryContext()) {
            app_state_.boot.mode = BootMode::Recovery;
            app_state_.boot.stage = BootStage::RecoveryMode;
            app_state_.boot.boot_in_progress = false;
            app_state_.screen = Screen::ErrorRecovery;

            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }

        settleLibrary();
        return BootRestoreResult{
            BootRestoreStatus::FallbackToLibrary,
            storage,
        };
    }

    if (restore.screen != Screen::Reading ||
        !restore.current_book.has_value()) {
        settleLibrary();
        return BootRestoreResult{
            BootRestoreStatus::LibraryReady,
            storage,
        };
    }

    if (!library_.get(*restore.current_book).has_value()) {
        if (!persistLibraryContext()) {
            app_state_.boot.mode = BootMode::Recovery;
            app_state_.boot.stage = BootStage::RecoveryMode;
            app_state_.boot.boot_in_progress = false;
            app_state_.screen = Screen::ErrorRecovery;

            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }

        app_state_.library.focused_book =
            restore.current_book;
        settleLibrary();
        return BootRestoreResult{
            BootRestoreStatus::FallbackToLibrary,
            storage,
        };
    }

    app_state_.screen = Screen::Library;

    const auto open_result =
        reader_runtime_.handle(
            OpenBookRequested{*restore.current_book}
        );

    if (open_result == ReaderRuntimeResult::Applied &&
        app_state_.screen == Screen::Reading) {
        app_state_.boot.mode = BootMode::Normal;
        app_state_.boot.stage = BootStage::Stable;
        app_state_.boot.boot_in_progress = false;

        return BootRestoreResult{
            BootRestoreStatus::ReadingRestored,
            storage,
        };
    }

    if (!persistLibraryContext()) {
        app_state_.boot.mode = BootMode::Recovery;
        app_state_.boot.stage = BootStage::RecoveryMode;
        app_state_.boot.boot_in_progress = false;
        app_state_.screen = Screen::ErrorRecovery;

        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

    app_state_.library.focused_book =
        restore.current_book;
    settleLibrary();

    return BootRestoreResult{
        BootRestoreStatus::FallbackToLibrary,
        storage,
    };
}

BootRestoreResult BootRestoreCoordinator::restoreContextOnly() {
    return restoreLoadedContext(
        StorageStartupResult{
            StorageStartupStatus::Ready,
            0,
            0,
        }
    );
}

BootRestoreResult BootRestoreCoordinator::run() {
    auto storage = storage_startup_.run();

    if (!storage.ready()) {
        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

    return restoreLoadedContext(storage);
}

} // namespace enku
