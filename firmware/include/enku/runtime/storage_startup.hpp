#pragma once

#include <cstdint>
#include <string>

#include "../core/app_state.hpp"
#include "../storage/book_file_store.hpp"
#include "../storage/book_import_service.hpp"
#include "../storage/cbor_library_service.hpp"
#include "settings_runtime.hpp"

namespace enku {

enum class StorageStartupStatus : std::uint8_t {
    Ready,
    RecoveryRequired,
    LibraryRecoveryRequired,
    StorageUnavailable,
    SettingsPersistenceFailure,
    CleanupFailed,
};

struct StorageStartupResult {
    StorageStartupStatus status{StorageStartupStatus::Ready};
    std::uint32_t stale_tmp_removed{0};
    std::uint32_t pending_tmp_preserved{0};

    bool ready() const {
        return status == StorageStartupStatus::Ready;
    }
};

class StorageStartupCoordinator {
public:
    StorageStartupCoordinator(
        AppState& app_state,
        SettingsRuntimeController& settings,
        CborLibraryService& library,
        BookFileStore& files,
        BookImportService& importer
    );

    StorageStartupResult run();

private:
    AppState& app_state_;
    SettingsRuntimeController& settings_;
    CborLibraryService& library_;
    BookFileStore& files_;
    BookImportService& importer_;

    static std::string filename(
        const std::string& path
    );

    StorageStartupResult enterRecovery(
        StorageStartupStatus status,
        std::uint32_t stale_removed,
        std::uint32_t pending_preserved
    );
};

} // namespace enku
