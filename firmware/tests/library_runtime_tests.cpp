#include "enku/reader/book_loader.hpp"
#include "enku/runtime/library_runtime.hpp"
#include "enku/runtime/library_search_runtime.hpp"
#include "enku/runtime/book_details_runtime.hpp"
#include "enku/runtime/book_finished_runtime.hpp"
#include "enku/storage/cbor_boot_loop_service.hpp"
#include "enku/storage/cbor_bookmark_service.hpp"
#include "enku/runtime/storage_startup.hpp"
#include "enku/runtime/boot_restore.hpp"
#include "enku/runtime/sleep_wake.hpp"
#include "enku/runtime/power_off.hpp"
#include "enku/runtime/input_dispatcher.hpp"
#include "enku/runtime/settings_runtime.hpp"
#include "enku/runtime/reader_runtime.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/cbor_settings_service.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/staged_book_import_service.hpp"
#include "enku/storage/stored_book_source_service.hpp"

#include <cassert>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
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
        return accept;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }

    bool accept{true};
    RefreshRequest last;
    std::uint32_t submitted{0};
};

class FakeNetworkService final : public NetworkService {
public:
    bool connected() const override {
        return connected_;
    }

    void disconnect() override {
        connected_ = false;
    }

    bool connected_{false};
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
        return state_;
    }

    WakeReason wakeReason() const override {
        return WakeReason::NavigationInput;
    }

    bool requestSuspend() override {
        state_ = DevicePowerState::Suspended;
        return true;
    }

    void requestPowerOff() override {
        state_ = DevicePowerState::PoweredOff;
    }

    DevicePowerState state_{DevicePowerState::Active};
};


BookRecord makeBook(
    std::string id,
    std::string title,
    std::string author,
    std::string path,
    std::string fingerprint,
    ReadingState state,
    std::uint64_t added,
    std::uint64_t opened
) {
    BookRecord record;
    record.book_id = std::move(id);
    record.format = BookFormat::Txt;
    record.metadata.title = std::move(title);
    record.metadata.author_display = author;
    record.metadata.authors = {std::move(author)};
    record.source_path = std::move(path);
    record.source_filename =
        record.metadata.title + ".txt";
    record.file_size = 100;
    record.fingerprint = std::move(fingerprint);
    record.reading_state = state;
    record.progress =
        state == ReadingState::Finished ? 1.0F : 0.0F;
    record.added_order = added;
    record.last_opened_order = opened;
    return record;
}

} // namespace

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-library-runtime-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);
    CborLibraryService library(state_files);
    assert(library.load() == LibraryStatus::Ok);

    const auto alpha = makeBook(
        "alpha",
        "Alpha",
        "Ada",
        "/books/alpha.txt",
        "fp-alpha",
        ReadingState::New,
        1,
        0
    );

    const auto beta = makeBook(
        "beta",
        "Beta",
        "Boris",
        "/books/beta.txt",
        "fp-beta",
        ReadingState::Reading,
        2,
        20
    );

    assert(
        book_files.write(
            alpha.source_path,
            "Alpha book text with enough words for pagination."
        ) == BookFileStatus::Ok
    );
    assert(
        book_files.write(
            beta.source_path,
            "Beta book text with enough words for pagination."
        ) == BookFileStatus::Ok
    );

    assert(library.upsert(alpha) == LibraryStatus::Ok);
    assert(library.upsert(beta) == LibraryStatus::Ok);

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
    CborBookmarkService bookmarks(state_files);
    CborAppContextService context(state_files);
    FakeRefreshService refresh;

    AppState app;
    app.screen = Screen::Library;
    app.library.sort = LibrarySort::RecentlyAdded;
    app.library.direction = SortDirection::Ascending;

    ReaderRuntimeController reader(
        app,
        loader,
        refresh,
        library,
        checkpoint,
        context,
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80}
    );

    BookImportService import_core(library);
    StagedBookImportService importer(
        book_files,
        import_core
    );

    BookDeleteService deleter(
        library,
        book_files,
        checkpoint,
        context,
        &bookmarks
    );

    CborSettingsService settings_service(state_files);
    SettingsRuntimeController settings(
        app,
        settings_service
    );
    assert(
        settings.loadAndApply() ==
        SettingsRuntimeStatus::DefaultsCreated
    );
    assert(
        settings.handle(
            LibrarySortChanged{
                LibrarySort::RecentlyAdded,
                SortDirection::Ascending,
            }
        ) == PersistStatus::Ok
    );

    LibraryRuntimeController runtime(
        app,
        library,
        reader,
        importer,
        deleter,
        settings,
        refresh
    );

    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().total_matches == 2);
    assert(runtime.page().items.size() == 2);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );
    assert(app.library.total_matches == 2);

    BookDetailsRuntime book_details(
        app,
        library,
        runtime,
        reader,
        checkpoint,
        deleter
    );

    BookFinishedRuntime book_finished(
        app,
        library,
        runtime,
        reader,
        checkpoint
    );

    // New book -> START.
    app.library.focused_book = "alpha";
    assert(
        book_details.handle(
            OpenFocusedBookDetailsRequested{}
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.screen == Screen::BookDetails);
    assert(
        std::string(
            book_details.primaryActionLabel()
        ) == "START"
    );
    assert(
        book_details.handle(
            LogicalAction::Back
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    // Reading book -> CONTINUE.
    app.library.focused_book = "beta";
    assert(
        book_details.handle(
            OpenFocusedBookDetailsRequested{}
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        std::string(
            book_details.primaryActionLabel()
        ) == "CONTINUE"
    );
    assert(
        book_details.handle(
            LogicalAction::Back
        ) == BookDetailsRuntimeResult::Applied
    );

    // Finished book -> READ AGAIN -> confirmation defaults to Cancel.
    assert(
        bookmarks.add(
            BookmarkRecord{
                SemanticPosition{
                    "beta",
                    "txt",
                    4,
                },
                "Keep me",
            }
        ) == BookmarkStatus::Ok
    );

    assert(
        checkpoint.checkpoint(
            "beta",
            SemanticPosition{
                "beta",
                "txt",
                8,
            },
            1.0F,
            ReadingState::Finished
        ) == PersistStatus::Ok
    );
    assert(
        library.updateSummary(
            "beta",
            ReadingState::Finished,
            1.0F,
            20
        ) == LibraryStatus::Ok
    );

    app.library.focused_book = "beta";
    assert(
        book_details.handle(
            OpenFocusedBookDetailsRequested{}
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        std::string(
            book_details.primaryActionLabel()
        ) == "READ AGAIN"
    );
    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        app.book_details.mode ==
        BookDetailsMode::RestartConfirm
    );
    assert(!app.book_details.confirm_restart);

    // Back from confirmation is non-destructive.
    assert(
        book_details.handle(
            LogicalAction::Back
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        app.book_details.mode ==
        BookDetailsMode::Details
    );

    // Confirm restart: toggle from Cancel to Restart, then apply.
    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        book_details.handle(
            LogicalAction::NavigateNext
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.book_details.confirm_restart);
    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.screen == Screen::Reading);

    ReaderCheckpoint erased_checkpoint;
    assert(
        checkpoint.load(
            "beta",
            erased_checkpoint
        ) == PersistStatus::NotFound
    );

    const auto restarted_beta =
        library.get("beta");
    assert(restarted_beta.has_value());
    assert(
        restarted_beta->reading_state ==
        ReadingState::Reading
    );
    assert(restarted_beta->progress == 0.0F);

    std::vector<BookmarkRecord> beta_bookmarks;
    assert(
        bookmarks.load(
            "beta",
            beta_bookmarks
        ) == BookmarkStatus::Ok
    );
    assert(beta_bookmarks.size() == 1);
    assert(beta_bookmarks[0].label == "Keep me");

    assert(
        reader.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(
        runtime.handle(
            LibraryRefreshRequested{}
        ) == LibraryRuntimeResult::Applied
    );

    // Book Finished: reaching past the last page opens the completion
    // state; Back returns to the final page and default Confirm returns to
    // Library.
    app.library.focused_book = "beta";
    assert(
        reader.handle(
            OpenBookRequested{"beta"}
        ) == ReaderRuntimeResult::Applied
    );

    while (true) {
        const auto result =
            reader.handle(PageNextRequested{});

        if (result == ReaderRuntimeResult::EndOfBook) {
            break;
        }

        assert(result == ReaderRuntimeResult::Applied);
    }

    assert(app.current_book_finished);
    assert(app.reading_progress == 1.0F);
    assert(
        book_finished.openFromReader() ==
        BookFinishedRuntimeResult::Applied
    );
    assert(app.screen == Screen::BookFinished);
    assert(
        app.book_finished.focus ==
        BookFinishedFocus::BackToLibrary
    );

    assert(
        book_finished.handle(
            LogicalAction::Back
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(app.screen == Screen::Reading);
    assert(app.current_book_finished);

    assert(
        reader.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::EndOfBook
    );
    assert(
        book_finished.openFromReader() ==
        BookFinishedRuntimeResult::Applied
    );
    assert(
        book_finished.handle(
            LogicalAction::Confirm
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );

    // Read again uses the same Cancel-first confirmation model and opens
    // the book from the beginning after an explicit confirmation.
    assert(
        reader.handle(
            OpenBookRequested{"beta"}
        ) == ReaderRuntimeResult::Applied
    );
    assert(
        reader.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::EndOfBook
    );
    assert(
        book_finished.openFromReader() ==
        BookFinishedRuntimeResult::Applied
    );
    assert(
        book_finished.handle(
            LogicalAction::NavigateNext
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(
        app.book_finished.focus ==
        BookFinishedFocus::ReadAgain
    );
    assert(
        book_finished.handle(
            LogicalAction::Confirm
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(app.book_finished.restart_confirm);
    assert(!app.book_finished.confirm_restart);
    assert(
        book_finished.handle(
            LogicalAction::NavigateNext
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(app.book_finished.confirm_restart);
    assert(
        book_finished.handle(
            LogicalAction::Confirm
        ) == BookFinishedRuntimeResult::Applied
    );
    assert(app.screen == Screen::Reading);
    assert(!app.current_book_finished);

    const auto restarted =
        library.get("beta");
    assert(restarted.has_value());
    assert(
        restarted->reading_state ==
        ReadingState::Reading
    );
    assert(restarted->progress == 0.0F);

    ReaderCheckpoint restarted_checkpoint;
    assert(
        checkpoint.load(
            "beta",
            restarted_checkpoint
        ) == PersistStatus::NotFound
    );

    assert(
        reader.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(
        runtime.handle(
            LibraryRefreshRequested{}
        ) == LibraryRuntimeResult::Applied
    );

    // Delete confirmation defaults to Cancel and only removes after
    // explicit confirmation.
    const auto detail_delete = makeBook(
        "detail-delete",
        "Delete Me",
        "Tester",
        "/books/detail-delete.txt",
        "fp-detail-delete",
        ReadingState::New,
        3,
        0
    );
    assert(
        book_files.write(
            detail_delete.source_path,
            "Temporary book for Book Details deletion."
        ) == BookFileStatus::Ok
    );
    assert(
        library.upsert(detail_delete) ==
        LibraryStatus::Ok
    );
    assert(
        bookmarks.add(
            BookmarkRecord{
                SemanticPosition{
                    "detail-delete",
                    "txt",
                    1,
                },
                "Delete with book",
            }
        ) == BookmarkStatus::Ok
    );
    assert(
        runtime.handle(
            LibraryRefreshRequested{}
        ) == LibraryRuntimeResult::Applied
    );

    app.library.focused_book = "detail-delete";
    assert(
        book_details.handle(
            OpenFocusedBookDetailsRequested{}
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        book_details.handle(
            LogicalAction::NavigateNext
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        app.book_details.focus ==
        BookDetailsFocus::DeleteBook
    );
    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        app.book_details.mode ==
        BookDetailsMode::DeleteConfirm
    );
    assert(!app.book_details.confirm_delete);

    assert(
        book_details.handle(
            LogicalAction::Back
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(library.get("detail-delete").has_value());

    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        book_details.handle(
            LogicalAction::NavigateNext
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.book_details.confirm_delete);
    assert(
        book_details.handle(
            LogicalAction::Confirm
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(!library.get("detail-delete").has_value());

    std::string deleted_bytes;
    assert(
        book_files.read(
            detail_delete.source_path,
            deleted_bytes
        ) == BookFileStatus::NotFound
    );

    std::vector<BookmarkRecord> deleted_bookmarks;
    assert(
        bookmarks.load(
            "detail-delete",
            deleted_bookmarks
        ) == BookmarkStatus::NotFound
    );

    app.library.limit = 1;
    app.library.offset = 0;
    app.library.focused_book.reset();

    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().offset == 0);
    assert(runtime.page().items[0].book_id == "alpha");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    assert(
        runtime.handle(LibraryFocusNextRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "beta");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );

    assert(
        runtime.handle(LibraryFocusPreviousRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 0);
    assert(runtime.page().items[0].book_id == "alpha");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    app.library.limit = 1;
    app.library.offset = 1;
    app.library.focused_book = "beta";
    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items[0].book_id == "beta");

    LibrarySearchRuntime library_search(
        app,
        runtime
    );
    CborBootLoopService boot_loop(state_files);
    StorageStartupCoordinator storage_startup(
        app,
        settings,
        library,
        book_files,
        import_core
    );
    BootRestoreCoordinator boot_restore(
        app,
        storage_startup,
        context,
        boot_loop,
        reader,
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
    PowerOffCoordinator power_off(
        app,
        library,
        checkpoint,
        context,
        network,
        power
    );
    InputDispatcher dispatcher(
        app,
        runtime,
        reader,
        sleep_wake,
        power_off,
        nullptr,
        nullptr,
        &library_search,
        &book_details,
        &book_finished
    );

    // Physical input also routes through the dispatcher while details
    // are open.
    app.library.focused_book = "alpha";
    assert(
        book_details.handle(
            OpenFocusedBookDetailsRequested{}
        ) == BookDetailsRuntimeResult::Applied
    );
    assert(
        dispatcher.handle(
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Click,
            }
        ) == InputDispatchResult::Applied
    );
    assert(
        app.book_details.focus ==
        BookDetailsFocus::DeleteBook
    );
    assert(
        dispatcher.handle(
            PhysicalInputEvent{
                PhysicalControl::Boot,
                PressType::Click,
            }
        ) == InputDispatchResult::Applied
    );
    assert(app.screen == Screen::Library);


    assert(
        library_search.open() ==
        LibrarySearchRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Search);
    assert(app.keyboard.open);

    assert(
        dispatcher.handle(
            PhysicalInputEvent{
                PhysicalControl::Down,
                PressType::Click,
            }
        ) == InputDispatchResult::Applied
    );
    assert(app.keyboard.focus_index == 1);

    assert(
        dispatcher.handle(
            PhysicalInputEvent{
                PhysicalControl::Up,
                PressType::Click,
            }
        ) == InputDispatchResult::Applied
    );
    assert(app.keyboard.focus_index == 0);

    app.keyboard.focus_index = 10;
    assert(
        library_search.handle(LogicalAction::Confirm) ==
        LibrarySearchRuntimeResult::Applied
    );
    app.keyboard.focus_index = 12;
    assert(
        library_search.handle(LogicalAction::Confirm) ==
        LibrarySearchRuntimeResult::Applied
    );
    app.keyboard.focus_index = 10;
    assert(
        library_search.handle(LogicalAction::Confirm) ==
        LibrarySearchRuntimeResult::Applied
    );
    assert(app.library.search_text == "ada");

    app.keyboard.focus_index = 30;
    assert(
        library_search.handle(LogicalAction::Confirm) ==
        LibrarySearchRuntimeResult::Applied
    );
    assert(!app.keyboard.open);
    assert(app.library.mode == LibraryQueryMode::Search);
    assert(runtime.page().total_matches == 1);
    assert(runtime.page().items[0].book_id == "alpha");

    assert(
        library_search.handle(
            OpenLibrarySearchRequested{}
        ) == LibrarySearchRuntimeResult::Applied
    );
    assert(app.keyboard.open);

    assert(
        dispatcher.handle(
            PhysicalInputEvent{
                PhysicalControl::Boot,
                PressType::Click,
            }
        ) == InputDispatchResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Browse);
    assert(!app.keyboard.open);
    assert(app.library.search_text.empty());
    assert(runtime.page().total_matches == 2);
    assert(app.library.offset == 1);
    assert(runtime.page().offset == 1);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "beta");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );

    app.library.limit = 1;
    assert(
        runtime.handle(
            LibrarySearchChanged{"a"}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Search);
    assert(runtime.page().total_matches == 2);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().offset == 0);
    assert(runtime.page().items[0].book_id == "alpha");

    assert(
        runtime.handle(LibraryFocusNextRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items[0].book_id == "beta");

    assert(
        runtime.handle(LibraryFocusPreviousRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 0);
    assert(runtime.page().items[0].book_id == "alpha");

    app.library.limit = 24;
    assert(
        runtime.handle(
            LibrarySearchChanged{""}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Browse);

    assert(
        runtime.handle(LibraryFocusNextRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );
    assert(refresh.last.reason == RefreshReason::FocusChanged);
    assert(refresh.last.refresh_class == RefreshClass::Region);

    app.library.limit = 1;
    app.library.offset = 1;
    app.library.focused_book = "beta";

    assert(
        runtime.handle(
            LibrarySortChanged{
                LibrarySort::Title,
                SortDirection::Ascending,
            }
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.offset == 0);
    assert(runtime.page().offset == 0);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "alpha");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    const auto offset_before_view =
        app.library.offset;
    const auto focus_before_view =
        app.library.focused_book;

    assert(
        runtime.handle(
            LibraryViewChanged{LibraryView::List}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.offset == offset_before_view);
    assert(app.library.focused_book == focus_before_view);
    assert(app.library.view == LibraryView::List);

    app.library.offset = 1;
    app.library.focused_book = "beta";

    assert(
        runtime.handle(
            LibraryFilterChanged{LibraryFilter::Reading}
        ) == LibraryRuntimeResult::Applied
    );

    GlobalSettings persisted_settings;
    assert(
        settings_service.load(persisted_settings) ==
        PersistStatus::Ok
    );
    assert(
        persisted_settings.library_filter ==
        LibraryFilter::Reading
    );
    assert(app.library.offset == 0);
    assert(runtime.page().offset == 0);
    assert(runtime.page().total_matches == 1);
    assert(runtime.page().items[0].book_id == "beta");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );

    assert(
        runtime.handle(
            LibraryViewChanged{LibraryView::List}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.view == LibraryView::List);

    assert(
        runtime.handle(
            LibrarySearchChanged{"Ada"}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Search);
    assert(runtime.page().total_matches == 1);
    assert(runtime.page().items[0].book_id == "alpha");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    assert(
        runtime.handle(
            LibrarySearchChanged{""}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Browse);

    assert(
        runtime.handle(
            LibraryFilterChanged{LibraryFilter::All}
        ) == LibraryRuntimeResult::Applied
    );

    app.library.limit = 1;
    app.library.offset = 1;
    app.library.focused_book = "beta";

    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "beta");
    assert(app.library.view == LibraryView::List);

    assert(
        runtime.handle(OpenFocusedBookRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(app.screen == Screen::Reading);
    assert(
        app.current_book ==
        std::optional<BookId>{"beta"}
    );
    assert(app.library.offset == 1);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );
    assert(app.library.view == LibraryView::List);

    assert(
        reader.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);
    assert(app.library.offset == 1);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );
    assert(app.library.view == LibraryView::List);

    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "beta");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );

    assert(
        settings.handle(
            OrientationChanged{
                Orientation::Landscape
            }
        ) == PersistStatus::Ok
    );
    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(app.orientation == Orientation::Landscape);
    assert(runtime.page().offset == 1);
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"beta"}
    );
    assert(app.library.view == LibraryView::List);

    app.library.limit = 24;

    // Deleting the only item on the last page must clamp
    // the offset back to the previous valid page.
    app.library.limit = 1;
    app.library.offset = 1;
    app.library.focused_book = "beta";
    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(runtime.page().offset == 1);
    assert(runtime.page().items[0].book_id == "beta");

    assert(
        runtime.handle(
            DeleteFocusedBookRequested{}
        ) == LibraryRuntimeResult::Applied
    );
    assert(app.library.offset == 0);
    assert(runtime.page().offset == 0);
    assert(runtime.page().total_matches == 1);
    assert(runtime.page().items.size() == 1);
    assert(runtime.page().items[0].book_id == "alpha");
    assert(
        app.library.focused_book ==
        std::optional<BookId>{"alpha"}
    );

    // Restore beta for the remaining import tests.
    assert(library.upsert(beta) == LibraryStatus::Ok);
    assert(
        book_files.write(
            beta.source_path,
            "Beta book text with enough words for pagination."
        ) == BookFileStatus::Ok
    );

    const std::string staged_path =
        "/system/tmp/gamma-upload.txt";
    assert(
        book_files.write(
            staged_path,
            "Gamma import contents."
        ) == BookFileStatus::Ok
    );

    assert(!app.import_active);

    app.library.limit = 1;
    assert(
        runtime.handle(
            LibrarySortChanged{
                LibrarySort::RecentlyAdded,
                SortDirection::Ascending,
            }
        ) == LibraryRuntimeResult::Applied
    );

    assert(
        runtime.handle(
            ImportRequested{
                staged_path,
                "Gamma.txt",
                3,
            }
        ) == LibraryRuntimeResult::Applied
    );

    assert(!app.import_active);
    assert(
        runtime.lastImportStatus() ==
        StagedImportStatus::Ok
    );
    assert(runtime.page().total_matches == 3);
    assert(app.library.focused_book.has_value());
    assert(runtime.page().items.size() == 1);
    assert(
        runtime.page().items[0].book_id ==
        *app.library.focused_book
    );
    assert(runtime.page().offset == 2);

    const auto imported =
        library.get(*app.library.focused_book);
    assert(imported.has_value());
    assert(imported->metadata.title == "Gamma");
    assert(imported->reading_state == ReadingState::New);

    std::string staged_bytes;
    assert(
        book_files.read(
            staged_path,
            staged_bytes
        ) == BookFileStatus::NotFound
    );

    // Exact duplicate reports a distinct result and leaves its staged source
    // available for UI/recovery handling.
    const std::string duplicate_stage =
        "/system/tmp/gamma-duplicate.txt";
    assert(
        book_files.write(
            duplicate_stage,
            "Gamma import contents."
        ) == BookFileStatus::Ok
    );

    assert(
        runtime.handle(
            ImportRequested{
                duplicate_stage,
                "Gamma Copy.txt",
                4,
            }
        ) == LibraryRuntimeResult::ImportDuplicate
    );

    assert(!app.import_active);
    assert(
        runtime.lastImportStatus() ==
        StagedImportStatus::Duplicate
    );
    assert(
        book_files.read(
            duplicate_stage,
            staged_bytes
        ) == BookFileStatus::Ok
    );

    const auto delete_id =
        *app.library.focused_book;
    const auto delete_record =
        library.get(delete_id);
    assert(delete_record.has_value());

    assert(
        runtime.handle(
            DeleteFocusedBookRequested{}
        ) == LibraryRuntimeResult::Applied
    );

    assert(!library.get(delete_id).has_value());
    assert(runtime.page().total_matches == 2);
    assert(
        book_files.read(
            delete_record->source_path,
            staged_bytes
        ) == BookFileStatus::NotFound
    );

    // Deleting down to an empty Library must leave a stable
    // empty state with offset 0 and no focused book.
    while (runtime.page().total_matches > 0U) {
        assert(app.library.focused_book.has_value());

        const auto result =
            runtime.handle(
                DeleteFocusedBookRequested{}
            );

        if (runtime.page().total_matches == 0U) {
            assert(result == LibraryRuntimeResult::Empty);
            break;
        }

        assert(result == LibraryRuntimeResult::Applied);
    }

    assert(runtime.page().total_matches == 0);
    assert(runtime.page().items.empty());
    assert(app.library.offset == 0);
    assert(!app.library.focused_book.has_value());

    assert(
        runtime.handle(LibraryFocusNextRequested{}) ==
        LibraryRuntimeResult::Empty
    );
    assert(
        runtime.handle(LibraryFocusPreviousRequested{}) ==
        LibraryRuntimeResult::Empty
    );
    assert(
        runtime.handle(OpenFocusedBookRequested{}) ==
        LibraryRuntimeResult::Ignored
    );

    std::filesystem::remove_all(root, ec);
    return 0;
}
