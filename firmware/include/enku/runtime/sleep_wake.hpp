#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../services/services.hpp"
#include "boot_restore.hpp"

namespace enku {

enum class SleepWakeStatus : std::uint8_t {
    Applied,
    Ignored,
    BusyImport,
    CannotSuspend,
    CheckpointFailed,
    ContextSaveFailed,
    LibraryUpdateFailed,
    SuspendFailed,
    WakeRestoreFailed,
};

class SleepWakeCoordinator {
public:
    SleepWakeCoordinator(
        AppState& app_state,
        LibraryService& library,
        ReaderCheckpointService& checkpoint,
        AppContextService& context,
        NetworkService& network,
        PowerService& power,
        BootRestoreCoordinator& boot_restore
    );

    SleepWakeStatus sleep();
    SleepWakeStatus wake();

private:
    AppState& app_state_;
    LibraryService& library_;
    ReaderCheckpointService& checkpoint_;
    AppContextService& context_;
    NetworkService& network_;
    PowerService& power_;
    BootRestoreCoordinator& boot_restore_;
};

} // namespace enku
