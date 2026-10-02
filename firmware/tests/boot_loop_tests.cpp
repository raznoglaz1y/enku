#include "enku/reader/book_loader.hpp"
#include "enku/runtime/boot_restore.hpp"
#include "enku/runtime/reader_runtime.hpp"
#include "enku/runtime/storage_startup.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_boot_loop_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/stored_book_source_service.hpp"

#include <cassert>
#include <filesystem>
#include <string_view>

using namespace enku;

namespace {

class FixedWidthMeasurer final : public TextMeasurer {
public:
    std::uint16_t measureWidthPx(
        std::string_view utf8,
        const TypographySettings&
    ) const override {
        return static_cast<std::uint16_t>(utf8.size() * 10U);
    }

    std::uint16_t lineHeightPx(
        const TypographySettings&
    ) const override {
        return 20;
    }
};

class FakeRefreshService final : public RefreshService {
public:
    bool busy() const override { return false; }

    bool submit(const RefreshRequest&) override {
        return true;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }
};

std::filesystem::path makeRoot() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-boot-loop-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return root;
}

} // namespace

int main() {
    const auto root = makeRoot();

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);

    CborLibraryService library(state_files);
    assert(library.load() == LibraryStatus::Ok);

    BookImportService importer(library);

    AppState app;
    StorageStartupCoordinator storage_startup(
        app,
        library,
        book_files,
        importer
    );

    StoredBookSourceService source(book_files);
    FixedWidthMeasurer measurer;
    ReaderBookLoader loader(
        library,
        source,
        measurer
    );

    CborReaderCheckpointService checkpoint(
        state_files
    );
    CborAppContextService context(state_files);
    CborBootLoopService boot_loop(state_files);
    FakeRefreshService refresh;

    ReaderRuntimeController runtime(
        app,
        loader,
        refresh,
        library,
        checkpoint,
        context,
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80}
    );

    BootRestoreCoordinator boot(
        app,
        storage_startup,
        context,
        boot_loop,
        runtime,
        library
    );

    // Simulate two previous cold boots that never reached Stable.
    BootLoopMarker marker;
    assert(
        boot_loop.beginBoot(marker) ==
        PersistStatus::Ok
    );
    assert(marker.incomplete_boot_count == 1);
    assert(!marker.stable);

    assert(
        boot_loop.beginBoot(marker) ==
        PersistStatus::Ok
    );
    assert(marker.incomplete_boot_count == 2);
    assert(!marker.stable);

    // Third incomplete boot crosses the threshold and must enter Recovery
    // before any automatic context restore.
    const auto blocked = boot.run();

    assert(
        blocked.status ==
        BootRestoreStatus::RecoveryRequired
    );
    assert(app.screen == Screen::ErrorRecovery);
    assert(app.boot.mode == BootMode::Recovery);
    assert(app.boot.stage == BootStage::RecoveryMode);
    assert(app.boot.incomplete_boot_count == 3);
    assert(loader.session() == nullptr);

    // Explicit recovery/service completion can clear the marker. The next
    // cold boot starts a new sequence and reaches a normal stable Library.
    assert(
        boot_loop.markStable() ==
        PersistStatus::Ok
    );

    app = AppState{};

    const auto recovered = boot.run();

    assert(
        recovered.status ==
        BootRestoreStatus::LibraryReady
    );
    assert(app.screen == Screen::Library);
    assert(app.boot.mode == BootMode::Normal);
    assert(app.boot.stage == BootStage::Stable);
    assert(app.boot.incomplete_boot_count == 0);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return 0;
}
