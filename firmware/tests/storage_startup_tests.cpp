#include "enku/runtime/storage_startup.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace enku;

namespace {

std::filesystem::path makeRoot(
    const std::string& name
) {
    const auto root =
        std::filesystem::temp_directory_path() / name;

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return root;
}

void removeRoot(
    const std::filesystem::path& root
) {
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

void corruptFile(
    const std::filesystem::path& path
) {
    std::fstream file(
        path,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );
    assert(file);

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    assert(size > 0);

    file.seekg(size - std::streamoff(1));
    char value = 0;
    file.read(&value, 1);
    assert(file);

    value ^= static_cast<char>(0xFF);

    file.seekp(size - std::streamoff(1));
    file.write(&value, 1);
    file.flush();
    assert(file);
}

} // namespace

int main() {
    // Clean storage: empty Library + no tmp directory is a valid normal boot.
    {
        const auto root =
            makeRoot("enku-storage-startup-clean");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        BookImportService importer(library);

        AppState app;
        app.screen = Screen::Boot;

        StorageStartupCoordinator startup(
            app,
            library,
            book_files,
            importer
        );

        const auto result = startup.run();

        assert(result.ready());
        assert(result.stale_tmp_removed == 0);
        assert(result.pending_tmp_preserved == 0);
        assert(app.screen == Screen::Library);
        assert(app.boot.mode == BootMode::Normal);
        assert(app.boot.stage == BootStage::Stable);
        assert(!app.boot.boot_in_progress);

        removeRoot(root);
    }

    // A tmp file whose content is already in Library is a stale artifact from
    // a completed transaction and can be removed automatically.
    {
        const auto root =
            makeRoot("enku-storage-startup-stale");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        BookImportService importer(library);

        const BookImportSource source{
            "/system/tmp/already-imported.txt",
            "already-imported.txt",
            "Already committed contents."
        };

        const auto imported = importer.import(source, 1);
        assert(imported.ok());

        assert(
            book_files.write(
                source.source_path,
                source.bytes
            ) == BookFileStatus::Ok
        );

        AppState app;
        StorageStartupCoordinator startup(
            app,
            library,
            book_files,
            importer
        );

        const auto result = startup.run();

        assert(result.ready());
        assert(result.stale_tmp_removed == 1);
        assert(result.pending_tmp_preserved == 0);

        std::string bytes;
        assert(
            book_files.read(
                source.source_path,
                bytes
            ) == BookFileStatus::NotFound
        );

        removeRoot(root);
    }

    // A valid but uncommitted upload is ambiguous after power loss. Preserve
    // it and enter Recovery instead of auto-importing or deleting it.
    {
        const auto root =
            makeRoot("enku-storage-startup-pending");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        BookImportService importer(library);

        const std::string staged_path =
            "/system/tmp/pending.txt";
        const std::string staged_bytes =
            "Uncommitted upload survives reboot.";

        assert(
            book_files.write(
                staged_path,
                staged_bytes
            ) == BookFileStatus::Ok
        );

        AppState app;
        StorageStartupCoordinator startup(
            app,
            library,
            book_files,
            importer
        );

        const auto result = startup.run();

        assert(
            result.status ==
            StorageStartupStatus::RecoveryRequired
        );
        assert(result.pending_tmp_preserved == 1);
        assert(app.screen == Screen::ErrorRecovery);
        assert(app.boot.mode == BootMode::Recovery);
        assert(app.boot.stage == BootStage::RecoveryMode);

        std::string read_back;
        assert(
            book_files.read(
                staged_path,
                read_back
            ) == BookFileStatus::Ok
        );
        assert(read_back == staged_bytes);

        removeRoot(root);
    }

    // A totally invalid Library generation is not silently treated as an
    // empty Library. Startup enters Recovery Mode and preserves book files.
    {
        const auto root =
            makeRoot("enku-storage-startup-library-corrupt");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        BookImportService importer(library);

        const BookImportSource source{
            "/books/existing.txt",
            "existing.txt",
            "Existing book contents."
        };

        assert(importer.import(source, 1).ok());

        const auto library_a =
            root / "system" / "library.a.cbor";
        assert(std::filesystem::exists(library_a));
        corruptFile(library_a);

        CborLibraryService reopened(state_files);
        BookImportService reopened_importer(reopened);

        AppState app;
        StorageStartupCoordinator startup(
            app,
            reopened,
            book_files,
            reopened_importer
        );

        const auto result = startup.run();

        assert(
            result.status ==
            StorageStartupStatus::LibraryRecoveryRequired
        );
        assert(app.screen == Screen::ErrorRecovery);
        assert(app.boot.mode == BootMode::Recovery);

        removeRoot(root);
    }

    return 0;
}
