#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../services/services.hpp"
#include "reader_runtime.hpp"
#include "storage_startup.hpp"

namespace enku {

enum class BootRestoreStatus : std::uint8_t {
    LibraryReady,
    ReadingRestored,
    FallbackToLibrary,
    RecoveryRequired,
};

struct BootRestoreResult {
    BootRestoreStatus status{BootRestoreStatus::LibraryReady};
    StorageStartupResult storage;
};

class BootRestoreCoordinator {
public:
    BootRestoreCoordinator(
        AppState& app_state,
        StorageStartupCoordinator& storage_startup,
        AppContextService& context,
        BootLoopService& boot_loop,
        ReaderRuntimeController& reader_runtime,
        LibraryService& library
    );

    BootRestoreResult run();
    BootRestoreResult restoreContextOnly();

private:
    AppState& app_state_;
    StorageStartupCoordinator& storage_startup_;
    AppContextService& context_;
    BootLoopService& boot_loop_;
    ReaderRuntimeController& reader_runtime_;
    LibraryService& library_;

    bool persistLibraryContext();
    void settleLibrary();
    bool markBootStable();
    BootRestoreResult restoreLoadedContext(
        const StorageStartupResult& storage
    );
};

} // namespace enku
