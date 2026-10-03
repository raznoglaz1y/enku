#include "enku/reader/book_loader.hpp"
#include "enku/runtime/power_off.hpp"
#include "enku/runtime/reader_runtime.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
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
        const RefreshRequest&
    ) override {
        return true;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }
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
        return true;
    }

    DevicePowerState powerState() const override {
        return state;
    }

    WakeReason wakeReason() const override {
        return WakeReason::ColdBoot;
    }

    bool requestSuspend() override {
        return false;
    }

    bool requestPowerOff() override {
        ++power_off_requests;
        state = DevicePowerState::PoweredOff;
        return true;
    }

    DevicePowerState state{DevicePowerState::Active};
    std::uint32_t power_off_requests{0};
};

std::filesystem::path makeRoot() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-power-off-test";

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
        "/books/power-off.txt";
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
    record.book_id = "power-off-book";
    record.format = BookFormat::Txt;
    record.metadata.title = "Power Off";
    record.metadata.author_display = "Unknown author";
    record.source_path = book_path;
    record.source_filename = "power-off.txt";
    record.file_size = text.size();
    record.fingerprint = "fp-power-off";
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
    FakeRefreshService refresh;

    AppState app;
    app.screen = Screen::Library;

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

    assert(
        runtime.handle(
            OpenBookRequested{"power-off-book"}
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

    FakeNetworkService network;
    FakePowerService power;

    PowerOffCoordinator coordinator(
        app,
        library,
        checkpoint,
        context,
        network,
        power
    );

    assert(
        coordinator.powerOff() ==
        PowerOffStatus::Applied
    );

    assert(!app.progress_dirty);
    assert(!network.connected());
    assert(network.disconnects == 1);
    assert(power.power_off_requests == 1);
    assert(
        power.powerState() ==
        DevicePowerState::PoweredOff
    );

    ReaderCheckpoint saved;
    assert(
        checkpoint.load(
            "power-off-book",
            saved
        ) == PersistStatus::Ok
    );
    assert(
        saved.position.text_offset ==
        expected_offset
    );

    AppRestoreContext restore;
    assert(context.load(restore) == PersistStatus::Ok);
    assert(restore.screen == Screen::Reading);
    assert(
        restore.current_book ==
        std::optional<BookId>{"power-off-book"}
    );

    const auto summary =
        library.get("power-off-book");
    assert(summary.has_value());
    assert(
        summary->reading_state ==
        ReadingState::Reading
    );

    // Powering off from temporary Library Search persists the stable
    // Browse origin, not the current Search result window.
    app.screen = Screen::Library;
    app.library.mode = LibraryQueryMode::Search;
    app.library.offset = 0;
    app.library.focused_book = "power-off-book";
    app.library.search_origin_valid = true;
    app.library.search_origin_offset = 7;
    app.library.search_origin_focused_book =
        BookId{"power-off-book"};

    assert(
        coordinator.powerOff() ==
        PowerOffStatus::Applied
    );

    AppRestoreContext search_restore;
    assert(
        context.load(search_restore) ==
        PersistStatus::Ok
    );
    assert(search_restore.screen == Screen::Library);
    assert(search_restore.library_offset == 7);
    assert(
        search_restore.library_focused_book ==
        std::optional<BookId>{"power-off-book"}
    );

    // Destructive import activity blocks graceful shutdown before PMU power
    // off can be requested.
    app.screen = Screen::Library;
    app.import_active = true;

    const auto power_off_before =
        power.power_off_requests;

    assert(
        coordinator.powerOff() ==
        PowerOffStatus::BusyImport
    );

    assert(
        power.power_off_requests ==
        power_off_before
    );

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    return 0;
}
