#include "enku/runtime/boot_restore.hpp"

#include <algorithm>

namespace enku {

BootRestoreCoordinator::BootRestoreCoordinator(
    AppState& app_state,
    StorageStartupCoordinator& storage_startup,
    AppContextService& context,
    BootLoopService& boot_loop,
    ReaderRuntimeController& reader_runtime,
    LibraryService& library
)
    : app_state_(app_state),
      storage_startup_(storage_startup),
      context_(context),
      boot_loop_(boot_loop),
      reader_runtime_(reader_runtime),
      library_(library) {}

bool BootRestoreCoordinator::persistLibraryContext() {
    return context_.save(
        AppRestoreContext{
            Screen::Library,
            std::nullopt,
            app_state_.library.offset,
            app_state_.library.focused_book,
        }
    ) == PersistStatus::Ok;
}

bool BootRestoreCoordinator::normalizeLibraryPosition(
    bool& changed
) {
    changed = false;

    const auto original_offset =
        app_state_.library.offset;
    const auto original_focus =
        app_state_.library.focused_book;

    LibraryQuery query;
    query.mode = LibraryQueryMode::Browse;
    query.filter = app_state_.library.filter;
    query.sort = app_state_.library.sort;
    query.direction = app_state_.library.direction;
    query.offset = app_state_.library.offset;
    query.limit = app_state_.library.limit;

    LibraryPage page;
    const auto status =
        library_.query(query, page);

    if (status != LibraryStatus::Ok) {
        return false;
    }

    if (page.total_matches == 0U) {
        app_state_.library.offset = 0;
        app_state_.library.focused_book.reset();

        changed =
            original_offset != 0U ||
            original_focus.has_value();
        return true;
    }

    if (page.items.empty()) {
        const auto limit =
            static_cast<std::uint32_t>(
                app_state_.library.limit
            );

        if (limit == 0U) {
            return false;
        }

        app_state_.library.offset =
            ((page.total_matches - 1U) /
             limit) *
            limit;

        query.offset =
            app_state_.library.offset;

        if (library_.query(query, page) !=
            LibraryStatus::Ok ||
            page.items.empty()) {
            return false;
        }
    }

    bool focus_valid = false;

    if (app_state_.library.focused_book.has_value()) {
        focus_valid =
            std::any_of(
                page.items.begin(),
                page.items.end(),
                [&](const BookRecord& record) {
                    return record.book_id ==
                        *app_state_.library.focused_book;
                }
            );
    }

    if (!focus_valid) {
        app_state_.library.focused_book =
            page.items.front().book_id;
    }

    changed =
        app_state_.library.offset !=
            original_offset ||
        app_state_.library.focused_book !=
            original_focus;

    return true;
}

bool BootRestoreCoordinator::markBootStable() {
    const auto status = boot_loop_.markStable();
    if (status != PersistStatus::Ok) {
        app_state_.boot.mode = BootMode::Recovery;
        app_state_.boot.stage = BootStage::RecoveryMode;
        app_state_.boot.boot_in_progress = false;
        app_state_.screen = Screen::ErrorRecovery;
        return false;
    }

    app_state_.boot.incomplete_boot_count = 0;
    return true;
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
    const StorageStartupResult& storage,
    bool finalize_boot_marker
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
        if (finalize_boot_marker &&
            !markBootStable()) {
            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }
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
        if (finalize_boot_marker &&
            !markBootStable()) {
            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }
        return BootRestoreResult{
            BootRestoreStatus::FallbackToLibrary,
            storage,
        };
    }

    app_state_.library.offset =
        restore.library_offset;
    app_state_.library.focused_book =
        restore.library_focused_book;

    bool library_position_changed = false;
    if (!normalizeLibraryPosition(
            library_position_changed
        )) {
        app_state_.boot.mode = BootMode::Recovery;
        app_state_.boot.stage = BootStage::RecoveryMode;
        app_state_.boot.boot_in_progress = false;
        app_state_.screen = Screen::ErrorRecovery;

        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

    if (restore.screen != Screen::Reading ||
        !restore.current_book.has_value()) {
        if (library_position_changed &&
            !persistLibraryContext()) {
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
        if (finalize_boot_marker &&
            !markBootStable()) {
            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }
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
        if (finalize_boot_marker &&
            !markBootStable()) {
            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }
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

        if (finalize_boot_marker &&
            !markBootStable()) {
            return BootRestoreResult{
                BootRestoreStatus::RecoveryRequired,
                storage,
            };
        }

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

    if (!markBootStable()) {
        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

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
        },
        false
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

    BootLoopMarker marker;
    const auto marker_status =
        boot_loop_.beginBoot(marker);

    if (marker_status != PersistStatus::Ok) {
        app_state_.boot.mode = BootMode::Recovery;
        app_state_.boot.stage = BootStage::RecoveryMode;
        app_state_.boot.boot_in_progress = false;
        app_state_.screen = Screen::ErrorRecovery;

        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

    app_state_.boot.incomplete_boot_count =
        marker.incomplete_boot_count;

    constexpr std::uint8_t kBootLoopThreshold = 3;

    if (marker.incomplete_boot_count >=
        kBootLoopThreshold) {
        app_state_.boot.mode = BootMode::Recovery;
        app_state_.boot.stage = BootStage::RecoveryMode;
        app_state_.boot.boot_in_progress = false;
        app_state_.screen = Screen::ErrorRecovery;

        return BootRestoreResult{
            BootRestoreStatus::RecoveryRequired,
            storage,
        };
    }

    return restoreLoadedContext(storage, true);
}

} // namespace enku
