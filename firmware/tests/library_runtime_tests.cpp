#include "enku/reader/book_loader.hpp"
#include "enku/runtime/library_runtime.hpp"
#include "enku/runtime/reader_runtime.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/staged_book_import_service.hpp"
#include "enku/storage/stored_book_source_service.hpp"

#include <cassert>
#include <filesystem>
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

    LibraryRuntimeController runtime(
        app,
        library,
        reader,
        importer,
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

    assert(
        runtime.handle(
            LibraryFilterChanged{LibraryFilter::Reading}
        ) == LibraryRuntimeResult::Applied
    );
    assert(runtime.page().total_matches == 1);
    assert(runtime.page().items[0].book_id == "beta");

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

    app.library.focused_book = "beta";

    assert(
        runtime.handle(OpenFocusedBookRequested{}) ==
        LibraryRuntimeResult::Applied
    );
    assert(app.screen == Screen::Reading);
    assert(
        app.current_book ==
        std::optional<BookId>{"beta"}
    );

    assert(
        reader.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(app.screen == Screen::Library);

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

    std::filesystem::remove_all(root, ec);
    return 0;
}
