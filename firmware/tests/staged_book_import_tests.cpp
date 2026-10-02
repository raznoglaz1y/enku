#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/staged_book_import_service.hpp"

#include <cassert>
#include <filesystem>
#include <optional>
#include <string>

using namespace enku;

namespace {

class FailingLibraryService final : public LibraryService {
public:
    LibraryStatus upsert(const BookRecord&) override {
        return LibraryStatus::PersistenceFailure;
    }

    LibraryStatus remove(const BookId&) override {
        return LibraryStatus::NotFound;
    }

    std::optional<BookRecord> get(
        const BookId&
    ) const override {
        return std::nullopt;
    }

    LibraryStatus query(
        const LibraryQuery&,
        LibraryPage&
    ) const override {
        return LibraryStatus::Ok;
    }

    std::optional<BookId> findByFingerprint(
        const std::string&
    ) const override {
        return std::nullopt;
    }

    LibraryStatus updateSummary(
        const BookId&,
        ReadingState,
        float,
        std::uint64_t
    ) override {
        return LibraryStatus::Ok;
    }
};

} // namespace

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-staged-import-test";

    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);

    PosixBookFileStore book_files(root);
    PosixStateFileStore state_files(root);
    CborLibraryService library(state_files);

    assert(library.load() == LibraryStatus::Ok);

    BookImportService importer(library);
    StagedBookImportService staged(book_files, importer);

    const std::string staged_path =
        "/system/tmp/example-upload.txt";
    const std::string bytes =
        "Alpha paragraph.\n\nBeta paragraph.";

    assert(
        book_files.write(staged_path, bytes) ==
        BookFileStatus::Ok
    );

    const auto imported = staged.import(
        staged_path,
        "Example Book.txt",
        100
    );

    assert(imported.ok());
    assert(!imported.book_id.empty());
    assert(
        imported.final_path ==
        "/books/" + imported.book_id + ".txt"
    );

    std::string read_back;
    assert(
        book_files.read(
            imported.final_path,
            read_back
        ) == BookFileStatus::Ok
    );
    assert(read_back == bytes);

    assert(
        book_files.read(staged_path, read_back) ==
        BookFileStatus::NotFound
    );

    const auto record =
        library.get(imported.book_id);
    assert(record.has_value());
    assert(
        record->source_path ==
        imported.final_path
    );
    assert(
        record->source_filename ==
        "Example Book.txt"
    );
    assert(record->reading_state == ReadingState::New);
    assert(record->progress == 0.0F);

    // Reopen the Library from disk to prove the staged transaction persisted
    // both the final book file and the Library index.
    CborLibraryService reopened(state_files);
    assert(reopened.load() == LibraryStatus::Ok);
    assert(reopened.get(imported.book_id).has_value());

    // Duplicate validation happens before a final /books write. The staged
    // upload stays in tmp so UI can report the duplicate deterministically.
    const std::string duplicate_stage =
        "/system/tmp/duplicate.txt";
    assert(
        book_files.write(duplicate_stage, bytes) ==
        BookFileStatus::Ok
    );

    const auto duplicate = staged.import(
        duplicate_stage,
        "Duplicate.txt",
        101
    );
    assert(
        duplicate.status ==
        StagedImportStatus::Duplicate
    );
    assert(
        book_files.read(duplicate_stage, read_back) ==
        BookFileStatus::Ok
    );

    // If Library commit fails after the final file was written, the final file
    // is rolled back and the staged upload remains available for retry.
    FailingLibraryService failing_library;
    BookImportService failing_importer(failing_library);
    StagedBookImportService failing_staged(
        book_files,
        failing_importer
    );

    const std::string failed_stage =
        "/system/tmp/commit-failure.txt";
    const std::string failed_bytes =
        "Unique content that cannot be committed.";

    assert(
        book_files.write(
            failed_stage,
            failed_bytes
        ) == BookFileStatus::Ok
    );

    const auto failed = failing_staged.import(
        failed_stage,
        "Commit Failure.txt",
        102
    );

    assert(
        failed.status ==
        StagedImportStatus::LibraryCommitFailed
    );

    assert(
        book_files.read(
            failed.final_path,
            read_back
        ) == BookFileStatus::NotFound
    );
    assert(
        book_files.read(
            failed_stage,
            read_back
        ) == BookFileStatus::Ok
    );

    std::filesystem::remove_all(root, cleanup_error);
    return 0;
}
