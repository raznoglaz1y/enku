#include "enku/reader/book_loader.hpp"
#include "enku/runtime/boot_restore.hpp"
#include "enku/runtime/reader_runtime.hpp"
#include "enku/runtime/sleep_wake.hpp"
#include "enku/runtime/storage_startup.hpp"
#include "enku/runtime/settings_runtime.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_boot_loop_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_settings_service.hpp"
#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/stored_book_source_service.hpp"

#include <cassert>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

using namespace enku;

namespace {

class FixedWidthMeasurer final : public TextMeasurer {
public:
    std::uint16_t measureWidthPx(
        std::string_view utf8,
        const TypographySettings&
    ) const override {
        return static_cast<std::uint16_t>(
            utf8.size() * 10U
        );
    }

    std::uint16_t lineHeightPx(
        const TypographySettings&
    ) const override {
        return 20;
    }
};

class FakeRefreshService final : public RefreshService {
public:
    bool busy() const override {
        return false;
    }

    bool submit(
        const RefreshRequest& request
    ) override {
        last = request;
        ++submitted;
        return true;
    }

    void cancelObsolete(
        std::uint32_t
    ) override {}

    RefreshStats stats() const override {
        return {};
    }

    RefreshRequest last;
    std::uint32_t submitted{0};
};

class FakeNetworkService final : public NetworkService {
public:
    bool connected() const override {
        return is_connected;
    }

    void disconnect() override {
        ++disconnects;
        is_connected = false;
    }

    bool is_connected{true};
    std::uint32_t disconnects{0};
};

class FakePowerService final : public PowerService {
public:
    BatteryState batteryState() const override {
        return {};
    }

    bool canSuspend() const override {
        return suspend_available;
    }

    DevicePowerState powerState() const override {
        return state;
    }

    WakeReason wakeReason() const override {
        return wake_reason;
    }

    bool requestSuspend() override {
        ++suspend_requests;

        if (!suspend_success) {
            return false;
        }

        state = DevicePowerState::Suspended;
        return true;
    }

    void requestPowerOff() override {
        state = DevicePowerState::PoweredOff;
    }

    bool suspend_available{true};
    bool suspend_success{true};
    DevicePowerState state{DevicePowerState::Active};
    WakeReason wake_reason{WakeReason::NavigationInput};
    std::uint32_t suspend_requests{0};
};

std::filesystem::path makeRoot() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-sleep-wake-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return root;
}

} // namespace

int main() {
    const auto root = makeRoot();

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);

    const std::string book_path =
        "/books/sleep-wake.txt";
    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
        "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega.";

    assert(
        book_files.write(book_path, text) ==
        BookFileStatus::Ok
    );

    CborLibraryService library(state_files);
    assert(library.load() == LibraryStatus::Ok);

    BookRecord record;
    record.book_id = "sleep-wake-book";
    record.format = BookFormat::Txt;
    record.metadata.title = "Sleep Wake";
    record.metadata.author_display = "Unknown author";
    record.source_path = book_path;
    record.source_filename = "sleep-wake.txt";
    record.file_size = text.size();
    record.fingerprint = "fp-sleep-wake";
    record.reading_state = ReadingState::New;
    record.added_order = 1;

    assert(
        library.upsert(record) ==
        LibraryStatus::Ok
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

    AppState app;
    app.screen = Screen::Library;

    const TypographySettings typography{
        16,
        1.0F,
        10,
    };
    const Viewport viewport{
        140,
        80,
    };

    ReaderRuntimeController runtime(
        app,
        loader,
        refresh,
        library,
        checkpoint,
        context,
        typography,
        viewport
    );

    assert(
        runtime.handle(
            OpenBookRequested{"sleep-wake-book"}
        ) == ReaderRuntimeResult::Applied
    );

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Applied
    );

    assert(app.reading_position.has_value());
    const auto expected_offset =
        app.reading_position->text_offset;
    assert(expected_offset > 0);
    assert(app.progress_dirty);

    BookImportService importer(library);
    CborSettingsService settings_service(state_files);
    SettingsRuntimeController settings(
        app,
        settings_service
    );
    StorageStartupCoordinator storage_startup(
        app,
        settings,
        library,
        book_files,
        importer
    );

    BootRestoreCoordinator boot_restore(
        app,
        storage_startup,
        context,
        boot_loop,
        runtime,
        library
    );

    FakeNetworkService network;
    FakePowerService power;

    SleepWakeCoordinator sleep_wake(
        app,
        library,
        checkpoint,
        context,
        network,
        power,
        boot_restore
    );

    assert(
        sleep_wake.sleep() ==
        SleepWakeStatus::Applied
    );

    assert(app.screen == Screen::Sleep);
    assert(!app.progress_dirty);
    assert(!network.connected());
    assert(network.disconnects == 1);
    assert(power.suspend_requests == 1);
    assert(
        power.powerState() ==
        DevicePowerState::Suspended
    );

    ReaderCheckpoint saved;
    assert(
        checkpoint.load(
            "sleep-wake-book",
            saved
        ) == PersistStatus::Ok
    );
    assert(
        saved.position.text_offset ==
        expected_offset
    );

    AppRestoreContext saved_context;
    assert(
        context.load(saved_context) ==
        PersistStatus::Ok
    );
    assert(saved_context.screen == Screen::Reading);
    assert(
        saved_context.current_book ==
        std::optional<BookId>{
            "sleep-wake-book"
        }
    );

    const auto summary =
        library.get("sleep-wake-book");
    assert(summary.has_value());
    assert(
        summary->reading_state ==
        ReadingState::Reading
    );
    assert(
        summary->progress ==
        app.reading_progress
    );

    assert(
        sleep_wake.wake() ==
        SleepWakeStatus::Applied
    );

    assert(app.screen == Screen::Reading);
    assert(
        app.current_book ==
        std::optional<BookId>{
            "sleep-wake-book"
        }
    );
    assert(app.reading_position.has_value());
    assert(
        app.reading_position->text_offset ==
        expected_offset
    );
    assert(app.boot.stage == BootStage::Stable);

    // Sleeping from temporary Library Search persists the stable
    // Browse origin instead of the Search result window.
    app.screen = Screen::Library;
    app.library.mode = LibraryQueryMode::Search;
    app.library.offset = 0;
    app.library.focused_book = "sleep-wake-book";
    app.library.search_origin_valid = true;
    app.library.search_origin_offset = 5;
    app.library.search_origin_focused_book =
        BookId{"sleep-wake-book"};
    power.suspend_available = true;
    power.suspend_success = true;

    assert(
        sleep_wake.sleep() ==
        SleepWakeStatus::Applied
    );

    AppRestoreContext search_sleep_context;
    assert(
        context.load(search_sleep_context) ==
        PersistStatus::Ok
    );
    assert(
        search_sleep_context.screen ==
        Screen::Library
    );
    assert(
        search_sleep_context.library_offset == 5
    );
    assert(
        search_sleep_context.library_focused_book ==
        std::optional<BookId>{"sleep-wake-book"}
    );

    // Unsafe import activity blocks automatic/user Sleep before any power
    // transition is attempted.
    app.screen = Screen::Library;
    app.import_active = true;
    const auto suspend_requests_before =
        power.suspend_requests;

    assert(
        sleep_wake.sleep() ==
        SleepWakeStatus::BusyImport
    );
    assert(
        power.suspend_requests ==
        suspend_requests_before
    );

    app.import_active = false;
    power.suspend_available = false;

    assert(
        sleep_wake.sleep() ==
        SleepWakeStatus::CannotSuspend
    );

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return 0;
}
