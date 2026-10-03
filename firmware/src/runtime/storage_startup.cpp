#include "enku/runtime/storage_startup.hpp"

#include <vector>

namespace enku {

StorageStartupCoordinator::StorageStartupCoordinator(
    AppState& app_state,
    SettingsRuntimeController& settings,
    CborLibraryService& library,
    BookFileStore& files,
    BookImportService& importer
)
    : app_state_(app_state),
      settings_(settings),
      library_(library),
      files_(files),
      importer_(importer) {}

std::string StorageStartupCoordinator::filename(
    const std::string& path
) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string::npos
        ? path
        : path.substr(slash + 1);
}

StorageStartupResult
StorageStartupCoordinator::enterRecovery(
    StorageStartupStatus status,
    std::uint32_t stale_removed,
    std::uint32_t pending_preserved
) {
    app_state_.boot.stage = BootStage::RecoveryMode;
    app_state_.boot.mode = BootMode::Recovery;
    app_state_.boot.boot_in_progress = false;
    app_state_.screen = Screen::ErrorRecovery;

    return StorageStartupResult{
        status,
        stale_removed,
        pending_preserved,
    };
}

StorageStartupResult StorageStartupCoordinator::run() {
    app_state_.boot.boot_in_progress = true;
    app_state_.boot.stage = BootStage::Storage;

    app_state_.boot.stage = BootStage::Persistence;

    const auto settings_status =
        settings_.loadAndApply();

    if (settings_status ==
        SettingsRuntimeStatus::PersistenceFailure) {
        return enterRecovery(
            StorageStartupStatus::SettingsPersistenceFailure,
            0,
            0
        );
    }

    const auto library_status = library_.load();

    if (library_status != LibraryStatus::Ok) {
        return enterRecovery(
            StorageStartupStatus::LibraryRecoveryRequired,
            0,
            0
        );
    }

    app_state_.boot.stage = BootStage::RecoveryDecision;

    if (app_state_.storage.removable !=
        RemovableStorageStatus::Ready) {
        app_state_.boot.stage = BootStage::FirstScreen;
        app_state_.boot.mode = BootMode::Normal;
        app_state_.screen = Screen::Library;
        app_state_.boot.stage = BootStage::Stable;
        app_state_.boot.boot_in_progress = false;

        return StorageStartupResult{
            StorageStartupStatus::Ready,
            0,
            0,
        };
    }

    std::vector<std::string> staged_paths;
    const auto list_status =
        files_.list("/system/tmp", staged_paths);

    if (list_status == BookFileStatus::IoError ||
        list_status == BookFileStatus::NoSpace) {
        return enterRecovery(
            StorageStartupStatus::StorageUnavailable,
            0,
            0
        );
    }

    std::uint32_t stale_removed = 0;
    std::uint32_t pending_preserved = 0;

    if (list_status == BookFileStatus::Ok) {
        for (const auto& staged_path : staged_paths) {
            std::string bytes;
            const auto read_status =
                files_.read(staged_path, bytes);

            if (read_status != BookFileStatus::Ok) {
                ++pending_preserved;
                continue;
            }

            const BookImportSource source{
                staged_path,
                filename(staged_path),
                bytes,
            };

            const auto prepared =
                importer_.prepare(source, 0);

            if (prepared.status ==
                BookImportStatus::Duplicate) {
                if (files_.remove(staged_path) !=
                    BookFileStatus::Ok) {
                    return enterRecovery(
                        StorageStartupStatus::CleanupFailed,
                        stale_removed,
                        pending_preserved
                    );
                }

                ++stale_removed;
                continue;
            }

            ++pending_preserved;
        }
    }

    if (pending_preserved > 0) {
        return enterRecovery(
            StorageStartupStatus::RecoveryRequired,
            stale_removed,
            pending_preserved
        );
    }

    app_state_.boot.stage = BootStage::FirstScreen;
    app_state_.boot.mode = BootMode::Normal;
    app_state_.screen = Screen::Library;
    app_state_.boot.stage = BootStage::Stable;
    app_state_.boot.boot_in_progress = false;

    return StorageStartupResult{
        StorageStartupStatus::Ready,
        stale_removed,
        0,
    };
}

} // namespace enku
