#include "enku/reader/book_loader.hpp"
#include "enku/runtime/library_runtime.hpp"
#include "enku/runtime/library_search_runtime.hpp"
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
        context
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

    app.library.limit = 24;
    app.library.offset = 0;
    app.library.focused_book.reset();
    assert(
        runtime.handle(LibraryRefreshRequested{}) ==
        LibraryRuntimeResult::Applied
    );

    LibrarySearchRuntime library_search(
        app,
        runtime
    );

    assert(
        library_search.open() ==
        LibrarySearchRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Search);
    assert(app.keyboard.open);

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
        library_search.handle(LogicalAction::Back) ==
        LibrarySearchRuntimeResult::Applied
    );
    assert(app.library.mode == LibraryQueryMode::Browse);
    assert(app.library.search_text.empty());
    assert(runtime.page().total_matches == 2);

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

    const std::string staged_path =
        "/system/tmp/gamma-upload.txt";
    assert(
        book_files.write(
            staged_path,
            "Gamma import contents."
        ) == BookFileStatus::Ok
    );

    assert(!app.import_active);

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

    std::filesystem::remove_all(root, ec);
    return 0;
}
