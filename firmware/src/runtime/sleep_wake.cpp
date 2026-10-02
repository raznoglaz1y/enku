#include "enku/runtime/sleep_wake.hpp"

namespace enku {

SleepWakeCoordinator::SleepWakeCoordinator(
    AppState& app_state,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    AppContextService& context,
    NetworkService& network,
    PowerService& power,
    BootRestoreCoordinator& boot_restore
)
    : app_state_(app_state),
      library_(library),
      checkpoint_(checkpoint),
      context_(context),
      network_(network),
      power_(power),
      boot_restore_(boot_restore) {}

SleepWakeStatus SleepWakeCoordinator::sleep() {
    if (app_state_.screen == Screen::Sleep) {
        return SleepWakeStatus::Ignored;
    }

    if (app_state_.import_active) {
        return SleepWakeStatus::BusyImport;
    }

    if (!power_.canSuspend()) {
        return SleepWakeStatus::CannotSuspend;
    }

    const auto previous_screen = app_state_.screen;

    if (app_state_.screen == Screen::Reading &&
        app_state_.current_book.has_value()) {
        const BookId book_id = *app_state_.current_book;
        const ReadingState reading_state =
            app_state_.current_book_finished
                ? ReadingState::Finished
                : ReadingState::Reading;

        if (app_state_.reading_position.has_value()) {
            const auto checkpoint_status =
                checkpoint_.checkpoint(
                    book_id,
                    *app_state_.reading_position,
                    app_state_.reading_progress,
                    reading_state
                );

            if (checkpoint_status != PersistStatus::Ok) {
                return SleepWakeStatus::CheckpointFailed;
            }

            app_state_.progress_dirty = false;
        }

        const auto record = library_.get(book_id);
        if (!record.has_value()) {
            return SleepWakeStatus::LibraryUpdateFailed;
        }

        if (library_.updateSummary(
                book_id,
                reading_state,
                app_state_.reading_progress,
                record->last_opened_order
            ) != LibraryStatus::Ok) {
            return SleepWakeStatus::LibraryUpdateFailed;
        }

        if (context_.save(
                AppRestoreContext{
                    Screen::Reading,
                    book_id,
                    app_state_.library.persistedOffset(),
                    app_state_.library.persistedFocusedBook(),
                }
            ) != PersistStatus::Ok) {
            return SleepWakeStatus::ContextSaveFailed;
        }
    } else {
        if (context_.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    app_state_.library.persistedOffset(),
                    app_state_.library.persistedFocusedBook(),
                }
            ) != PersistStatus::Ok) {
            return SleepWakeStatus::ContextSaveFailed;
        }
    }

    if (network_.connected()) {
        network_.disconnect();
    }

    app_state_.screen = Screen::Sleep;

    if (!power_.requestSuspend()) {
        app_state_.screen = previous_screen;
        return SleepWakeStatus::SuspendFailed;
    }

    return SleepWakeStatus::Applied;
}

SleepWakeStatus SleepWakeCoordinator::wake() {
    if (app_state_.screen != Screen::Sleep &&
        power_.powerState() != DevicePowerState::Suspended) {
        return SleepWakeStatus::Ignored;
    }

    const auto restored =
        boot_restore_.restoreContextOnly();

    if (restored.status ==
            BootRestoreStatus::ReadingRestored ||
        restored.status ==
            BootRestoreStatus::LibraryReady ||
        restored.status ==
            BootRestoreStatus::FallbackToLibrary) {
        return SleepWakeStatus::Applied;
    }

    return SleepWakeStatus::WakeRestoreFailed;
}

} // namespace enku
