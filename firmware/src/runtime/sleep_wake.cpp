#include "enku/runtime/sleep_wake.hpp"
#include "enku/runtime/reader_state_flush.hpp"

namespace enku {

SleepWakeCoordinator::SleepWakeCoordinator(
    AppState& app_state,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    AppContextService& context,
    NetworkLifecycleCoordinator& network_lifecycle,
    PowerService& power,
    BootRestoreCoordinator& boot_restore
)
    : app_state_(app_state),
      library_(library),
      checkpoint_(checkpoint),
      context_(context),
      network_lifecycle_(network_lifecycle),
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

    ReaderStateFlushCoordinator flush(
        app_state_,
        library_,
        checkpoint_,
        context_
    );

    switch (flush.flush(
        ReaderStateFlushTarget::PreserveReading
    )) {
        case ReaderStateFlushStatus::Applied:
            break;
        case ReaderStateFlushStatus::CheckpointFailed:
            return SleepWakeStatus::CheckpointFailed;
        case ReaderStateFlushStatus::LibraryUpdateFailed:
            return SleepWakeStatus::LibraryUpdateFailed;
        case ReaderStateFlushStatus::ContextSaveFailed:
            return SleepWakeStatus::ContextSaveFailed;
    }

    network_lifecycle_.disconnectForSuspend();

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
        // Network recovery is best-effort: Wi-Fi failure must never trap
        // the user on wake after the reading/library context is restored.
        network_lifecycle_.resume();
        return SleepWakeStatus::Applied;
    }

    return SleepWakeStatus::WakeRestoreFailed;
}

} // namespace enku
