#include "enku/reader/book_loader.hpp"
#include "enku/runtime/boot_restore.hpp"
#include "enku/runtime/reader_runtime.hpp"
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
#include <vector>

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

class InvalidCheckpointService final
    : public ReaderCheckpointService {
public:
    PersistStatus load(
        const BookId&,
        ReaderCheckpoint&
    ) override {
        return PersistStatus::InvalidRecord;
    }

    PersistStatus checkpoint(
        const BookId&,
        const SemanticPosition&,
        float,
        ReadingState
    ) override {
        return PersistStatus::InvalidRecord;
    }
};

class FakeRefreshService final : public RefreshService {
public:
    bool busy() const override {
        return false;
    }

    bool submit(const RefreshRequest& request) override {
        last = request;
        ++submitted;
        return true;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }

    RefreshRequest last;
    std::uint32_t submitted{0};
};

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

std::uint32_t legacyCrc32(
    const std::vector<std::uint8_t>& bytes
) {
    std::uint32_t crc = 0xFFFFFFFFU;

    for (const auto byte : bytes) {
        crc ^= byte;

        for (int bit = 0; bit < 8; ++bit) {
            const auto mask =
                static_cast<std::uint32_t>(
                    -(static_cast<std::int32_t>(crc & 1U))
                );
            crc =
                (crc >> 1U) ^
                (0xEDB88320U & mask);
        }
    }

    return ~crc;
}

void appendLegacyUnsigned(
    std::vector<std::uint8_t>& out,
    std::uint64_t value
) {
    if (value < 24U) {
        out.push_back(
            static_cast<std::uint8_t>(value)
        );
        return;
    }

    if (value <= 0xFFU) {
        out.push_back(0x18U);
        out.push_back(
            static_cast<std::uint8_t>(value)
        );
        return;
    }

    if (value <= 0xFFFFU) {
        out.push_back(0x19U);
        out.push_back(
            static_cast<std::uint8_t>(
                value >> 8U
            )
        );
        out.push_back(
            static_cast<std::uint8_t>(value)
        );
        return;
    }

    out.push_back(0x1AU);
    for (int shift = 24;
         shift >= 0;
         shift -= 8) {
        out.push_back(
            static_cast<std::uint8_t>(
                value >> shift
            )
        );
    }
}

std::vector<std::uint8_t> makeLegacyV1LibraryContext() {
    std::vector<std::uint8_t> payload;
    payload.push_back(0x82U);
    appendLegacyUnsigned(
        payload,
        static_cast<std::uint8_t>(
            Screen::Library
        )
    );
    payload.push_back(0xF6U);

    std::vector<std::uint8_t> out;
    out.push_back(0x85U);
    appendLegacyUnsigned(out, 1U);
    appendLegacyUnsigned(out, 3U);
    appendLegacyUnsigned(out, 1U);

    if (payload.size() < 24U) {
        out.push_back(
            static_cast<std::uint8_t>(
                0x40U | payload.size()
            )
        );
    } else {
        out.push_back(0x58U);
        out.push_back(
            static_cast<std::uint8_t>(
                payload.size()
            )
        );
    }

    out.insert(
        out.end(),
        payload.begin(),
        payload.end()
    );
    appendLegacyUnsigned(
        out,
        legacyCrc32(payload)
    );

    return out;
}

BookRecord makeBook(
    const std::string& book_id,
    const std::string& path,
    const std::string& fingerprint
) {
    BookRecord record;
    record.book_id = book_id;
    record.format = BookFormat::Txt;
    record.metadata.title = "Boot Restore";
    record.metadata.author_display = "Unknown author";
    record.source_path = path;
    record.source_filename = "boot-restore.txt";
    record.file_size = 0;
    record.fingerprint = fingerprint;
    record.reading_state = ReadingState::Reading;
    record.progress = 0.0F;
    record.added_order = 1;
    record.last_opened_order = 1;
    return record;
}

} // namespace

int main() {
    const TypographySettings typography{16, 1.0F, 10};
    const Viewport viewport{140, 80};

    // Schema v1 app-context records remain readable after the v2 upgrade.
    {
        const auto root =
            makeRoot("enku-app-context-v1-compat");

        PosixStateFileStore state_files(root);
        assert(
            state_files.write(
                "/system/context.a.cbor",
                makeLegacyV1LibraryContext()
            ) == StateFileStatus::Ok
        );

        CborAppContextService context(state_files);
        AppRestoreContext restored;

        assert(
            context.load(restored) ==
            PersistStatus::Ok
        );
        assert(restored.screen == Screen::Library);
        assert(!restored.current_book.has_value());
        assert(restored.library_offset == 0);
        assert(
            !restored.library_focused_book.has_value()
        );

        removeRoot(root);
    }

    // Power loss while Reading: persistent app context + checkpoint restore the
    // same book and semantic offset after constructing a completely new
    // runtime graph.
    {
        const auto root =
            makeRoot("enku-boot-restore-reading");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);

        const std::string book_path =
            "/books/book-restore.txt";
        const std::string text =
            "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
            "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega.";

        assert(
            book_files.write(book_path, text) ==
            BookFileStatus::Ok
        );

        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        auto record = makeBook(
            "book-restore",
            book_path,
            "fp-boot-restore"
        );
        record.file_size = text.size();

        assert(
            library.upsert(record) ==
            LibraryStatus::Ok
        );

        auto other = makeBook(
            "other-book",
            "/books/other-book.txt",
            "fp-other-book"
        );
        other.metadata.title = "Other";
        other.added_order = 1;
        record.added_order = 2;
        assert(
            library.upsert(record) ==
            LibraryStatus::Ok
        );
        assert(
            library.upsert(other) ==
            LibraryStatus::Ok
        );

        CborSettingsService initial_settings_store(state_files);
        GlobalSettings initial_settings;
        initial_settings.library_sort =
            LibrarySort::RecentlyAdded;
        initial_settings.library_direction =
            SortDirection::Ascending;
        assert(
            initial_settings_store.save(initial_settings) ==
            PersistStatus::Ok
        );

        std::uint64_t saved_offset = 0;

        {
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
            app.library.limit = 1;
            app.library.offset = 1;
            app.library.focused_book =
                BookId{"book-restore"};

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
                    OpenBookRequested{"book-restore"}
                ) == ReaderRuntimeResult::Applied
            );

            assert(app.screen == Screen::Reading);
            assert(
                runtime.handle(PageNextRequested{}) ==
                ReaderRuntimeResult::Applied
            );
            assert(app.reading_position.has_value());

            saved_offset =
                app.reading_position->text_offset;
            assert(saved_offset > 0);

            // Model a periodic/safe progress checkpoint immediately before a
            // power loss. The app context already says Reading.
            assert(
                checkpoint.checkpoint(
                    "book-restore",
                    *app.reading_position,
                    app.reading_progress,
                    ReadingState::Reading
                ) == PersistStatus::Ok
            );
        }

        // New objects model a cold reboot.
        CborLibraryService rebooted_library(state_files);
        BookImportService importer(rebooted_library);

        // Rebuild with one authoritative AppState for the real restore run.
        AppState boot_app;
        boot_app.library.limit = 1;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            boot_app,
            settings_service
        );
        StorageStartupCoordinator startup(
            boot_app,
            settings,
            rebooted_library,
            book_files,
            importer
        );

        StoredBookSourceService source(book_files);
        FixedWidthMeasurer measurer;
        ReaderBookLoader loader(
            rebooted_library,
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
            boot_app,
            loader,
            refresh,
            rebooted_library,
            checkpoint,
            context,
            typography,
            viewport
        );

        BootRestoreCoordinator boot(
            boot_app,
            startup,
            context,
            boot_loop,
            runtime,
            rebooted_library
        );

        const auto restored = boot.run();

        assert(
            restored.status ==
            BootRestoreStatus::ReadingRestored
        );
        assert(boot_app.screen == Screen::Reading);
        assert(
            boot_app.current_book ==
            std::optional<BookId>{"book-restore"}
        );
        assert(boot_app.reading_position.has_value());
        assert(
            boot_app.reading_position->text_offset ==
            saved_offset
        );
        assert(boot_app.boot.stage == BootStage::Stable);
        assert(!boot_app.boot.boot_in_progress);
        assert(boot_app.library.offset == 1);
        assert(
            boot_app.library.focused_book ==
            std::optional<BookId>{"book-restore"}
        );

        removeRoot(root);
    }

    // Library context also restores the visible window and focus across
    // a cold reboot.
    {
        const auto root =
            makeRoot("enku-boot-restore-library-position");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        const auto record = makeBook(
            "library-focus",
            "/books/library-focus.txt",
            "fp-library-focus"
        );

        assert(
            library.upsert(record) ==
            LibraryStatus::Ok
        );

        CborAppContextService context(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    0,
                    BookId{"library-focus"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
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
        CborBootLoopService boot_loop(state_files);
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::LibraryReady
        );
        assert(app.screen == Screen::Library);
        assert(app.library.offset == 0);
        assert(
            app.library.focused_book ==
            std::optional<BookId>{"library-focus"}
        );

        AppRestoreContext restored_context;
        assert(
            context.load(restored_context) ==
            PersistStatus::Ok
        );
        assert(restored_context.library_offset == 0);
        assert(
            restored_context.library_focused_book ==
            std::optional<BookId>{"library-focus"}
        );

        removeRoot(root);
    }

    // Stale Library offset/focus are normalized against the current
    // post-startup Library contents.
    {
        const auto root =
            makeRoot("enku-boot-restore-library-normalize");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        auto first = makeBook(
            "first",
            "/books/first.txt",
            "fp-first"
        );
        first.metadata.title = "First";
        first.added_order = 1;

        auto second = makeBook(
            "second",
            "/books/second.txt",
            "fp-second"
        );
        second.metadata.title = "Second";
        second.added_order = 2;

        assert(library.upsert(first) == LibraryStatus::Ok);
        assert(library.upsert(second) == LibraryStatus::Ok);

        CborAppContextService context(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    99,
                    BookId{"deleted-focus"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        GlobalSettings library_settings;
        library_settings.library_sort =
            LibrarySort::RecentlyAdded;
        library_settings.library_direction =
            SortDirection::Ascending;
        assert(
            settings_service.save(library_settings) ==
            PersistStatus::Ok
        );

        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
            library,
            book_files,
            importer
        );

        app.library.limit = 1;

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
        CborBootLoopService boot_loop(state_files);
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::LibraryReady
        );
        assert(app.screen == Screen::Library);
        assert(app.library.offset == 1);
        assert(
            app.library.focused_book ==
            std::optional<BookId>{"second"}
        );

        AppRestoreContext normalized;
        assert(
            context.load(normalized) ==
            PersistStatus::Ok
        );
        assert(normalized.library_offset == 1);
        assert(
            normalized.library_focused_book ==
            std::optional<BookId>{"second"}
        );

        removeRoot(root);
    }

    // Empty Library normalizes stale restore state to offset 0 / no focus
    // without entering recovery mode.
    {
        const auto root =
            makeRoot("enku-boot-restore-empty-library");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        CborAppContextService context(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    48,
                    BookId{"gone"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
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
        CborBootLoopService boot_loop(state_files);
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::LibraryReady
        );
        assert(app.screen == Screen::Library);
        assert(app.library.offset == 0);
        assert(!app.library.focused_book.has_value());

        removeRoot(root);
    }

    // If the Reading context points to a book removed from the Library
    // index, fallback keeps the normalized real Library focus instead of
    // resurrecting the stale book id.
    {
        const auto root =
            makeRoot("enku-boot-restore-missing-index-record");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        const auto survivor = makeBook(
            "survivor",
            "/books/survivor.txt",
            "fp-survivor"
        );
        assert(
            library.upsert(survivor) ==
            LibraryStatus::Ok
        );

        CborAppContextService context(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    BookId{"removed-book"},
                    0,
                    BookId{"removed-book"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
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
        CborBootLoopService boot_loop(state_files);
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::FallbackToLibrary
        );
        assert(app.screen == Screen::Library);
        assert(
            app.library.focused_book ==
            std::optional<BookId>{"survivor"}
        );
        assert(app.library.offset == 0);

        AppRestoreContext safe;
        assert(
            context.load(safe) ==
            PersistStatus::Ok
        );
        assert(safe.screen == Screen::Library);
        assert(
            safe.library_focused_book ==
            std::optional<BookId>{"survivor"}
        );

        removeRoot(root);
    }

    // If the previously-open book disappears, restore must fall back to
    // Library and rewrite the safe context so the next reboot does not retry
    // the same broken auto-open forever.
    {
        const auto root =
            makeRoot("enku-boot-restore-missing-source");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        const auto record = makeBook(
            "missing-book",
            "/books/missing.txt",
            "fp-missing"
        );

        assert(
            library.upsert(record) ==
            LibraryStatus::Ok
        );

        CborAppContextService context(state_files);
        CborBootLoopService boot_loop(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    BookId{"missing-book"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );

        StorageStartupCoordinator startup(
            app,
            settings,
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
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::FallbackToLibrary
        );
        assert(app.screen == Screen::Library);
        assert(
            app.library.focused_book ==
            std::optional<BookId>{"missing-book"}
        );
        assert(!app.current_book.has_value());
        assert(loader.session() == nullptr);

        AppRestoreContext safe;
        assert(context.load(safe) == PersistStatus::Ok);
        assert(safe.screen == Screen::Library);
        assert(!safe.current_book.has_value());

        removeRoot(root);
    }


    // Corrupt app-context slots must be rewritten to a safe Library context
    // instead of trapping boot in recovery.
    {
        const auto root =
            makeRoot("enku-boot-restore-corrupt-context");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);

        const std::vector<std::uint8_t> garbage{
            0xDEU,
            0xADU,
            0xBEU,
            0xEFU,
        };

        assert(
            state_files.write(
                "/system/context.a.cbor",
                garbage
            ) == StateFileStatus::Ok
        );
        assert(
            state_files.write(
                "/system/context.b.cbor",
                garbage
            ) == StateFileStatus::Ok
        );

        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
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
            typography,
            viewport
        );

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::FallbackToLibrary
        );
        assert(app.screen == Screen::Library);
        assert(app.boot.stage == BootStage::Stable);

        AppRestoreContext safe;
        assert(context.load(safe) == PersistStatus::Ok);
        assert(safe.screen == Screen::Library);
        assert(!safe.current_book.has_value());

        removeRoot(root);
    }

    // A corrupt reader checkpoint must not prevent the book's Library from
    // becoming usable. Boot falls back and rewrites app context to Library.
    {
        const auto root =
            makeRoot("enku-boot-restore-corrupt-checkpoint");

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);

        const std::string book_path =
            "/books/corrupt-checkpoint.txt";
        const std::string text =
            "Alpha beta gamma delta epsilon zeta eta theta.";

        assert(
            book_files.write(
                book_path,
                text
            ) == BookFileStatus::Ok
        );

        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        auto record = makeBook(
            "corrupt-checkpoint",
            book_path,
            "fp-corrupt-checkpoint"
        );
        record.file_size = text.size();

        assert(
            library.upsert(record) ==
            LibraryStatus::Ok
        );

        CborAppContextService context(state_files);
        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    BookId{"corrupt-checkpoint"},
                    0,
                    BookId{"corrupt-checkpoint"},
                }
            ) == PersistStatus::Ok
        );

        BookImportService importer(library);
        AppState app;
        CborSettingsService settings_service(state_files);
        SettingsRuntimeController settings(
            app,
            settings_service
        );
        StorageStartupCoordinator startup(
            app,
            settings,
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
        InvalidCheckpointService checkpoint;
        CborBootLoopService boot_loop(state_files);
        FakeRefreshService refresh;

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

        BootRestoreCoordinator boot(
            app,
            startup,
            context,
            boot_loop,
            runtime,
            library
        );

        const auto result = boot.run();

        assert(
            result.status ==
            BootRestoreStatus::FallbackToLibrary
        );
        assert(app.screen == Screen::Library);
        assert(
            app.library.focused_book ==
            std::optional<BookId>{
                "corrupt-checkpoint"
            }
        );
        assert(!app.current_book.has_value());
        assert(loader.session() == nullptr);

        AppRestoreContext safe;
        assert(context.load(safe) == PersistStatus::Ok);
        assert(safe.screen == Screen::Library);
        assert(!safe.current_book.has_value());

        removeRoot(root);
    }

    return 0;
}
