#include "enku/storage/book_delete_service.hpp"
#include "enku/storage/cbor_app_context_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_reader_checkpoint.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace enku;

namespace {

class MemoryStateFileStore final : public StateFileStore {
public:
    StateFileStatus read(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    ) override {
        const auto it = files.find(path);
        if (it == files.end()) {
            return StateFileStatus::NotFound;
        }
        bytes = it->second;
        return StateFileStatus::Ok;
    }

    StateFileStatus write(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    ) override {
        files[path] = bytes;
        return StateFileStatus::Ok;
    }

    StateFileStatus remove(
        const std::string& path
    ) override {
        if (fail_checkpoint_remove &&
            path.find("/system/state/") == 0) {
            return StateFileStatus::IoError;
        }

        const auto erased = files.erase(path);
        return erased > 0
            ? StateFileStatus::Ok
            : StateFileStatus::NotFound;
    }

    std::map<std::string, std::vector<std::uint8_t>> files;
    bool fail_checkpoint_remove{false};
};

class MemoryBookFileStore final : public BookFileStore {
public:
    BookFileStatus read(
        const std::string& path,
        std::string& bytes
    ) override {
        const auto it = files.find(path);
        if (it == files.end()) {
            return BookFileStatus::NotFound;
        }
        bytes = it->second;
        return BookFileStatus::Ok;
    }

    BookFileStatus write(
        const std::string& path,
        const std::string& bytes
    ) override {
        files[path] = bytes;
        return BookFileStatus::Ok;
    }

    BookFileStatus remove(
        const std::string& path
    ) override {
        if (fail_remove) {
            return BookFileStatus::IoError;
        }

        const auto erased = files.erase(path);
        return erased > 0
            ? BookFileStatus::Ok
            : BookFileStatus::NotFound;
    }

    BookFileStatus list(
        const std::string&,
        std::vector<std::string>& paths
    ) override {
        paths.clear();
        return BookFileStatus::Ok;
    }

    std::map<std::string, std::string> files;
    bool fail_remove{false};
};

BookRecord makeBook(
    std::string id,
    std::string path
) {
    BookRecord record;
    record.book_id = std::move(id);
    record.format = BookFormat::Txt;
    record.metadata.title = "Delete Me";
    record.metadata.author_display = "Unknown author";
    record.source_path = std::move(path);
    record.source_filename = "delete-me.txt";
    record.file_size = 123;
    record.fingerprint = "fp-delete";
    record.reading_state = ReadingState::Reading;
    record.progress = 0.5F;
    record.added_order = 1;
    record.last_opened_order = 2;
    return record;
}

void seedCheckpoint(
    CborReaderCheckpointService& checkpoint,
    const BookId& book_id
) {
    assert(
        checkpoint.checkpoint(
            book_id,
            SemanticPosition{
                book_id,
                "txt:body",
                77,
            },
            0.5F,
            ReadingState::Reading
        ) == PersistStatus::Ok
    );
}

} // namespace

int main() {
    // Successful delete removes Library entry, source, checkpoint and resets
    // a Reading safe-context that points at the deleted book.
    {
        const auto root =
            std::filesystem::temp_directory_path() /
            "enku-book-delete-success";

        std::error_code ec;
        std::filesystem::remove_all(root, ec);

        PosixStateFileStore state_files(root);
        PosixBookFileStore book_files(root);
        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        CborReaderCheckpointService checkpoint(state_files);
        CborAppContextService context(state_files);

        const auto record = makeBook(
            "delete-success",
            "/books/delete-success.txt"
        );

        assert(library.upsert(record) == LibraryStatus::Ok);
        assert(
            book_files.write(
                record.source_path,
                "Book contents"
            ) == BookFileStatus::Ok
        );

        seedCheckpoint(checkpoint, record.book_id);

        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    record.book_id,
                }
            ) == PersistStatus::Ok
        );

        BookDeleteService deleter(
            library,
            book_files,
            checkpoint,
            context
        );

        assert(
            deleter.remove(record.book_id) ==
            BookDeleteStatus::Ok
        );

        assert(!library.get(record.book_id).has_value());

        std::string bytes;
        assert(
            book_files.read(
                record.source_path,
                bytes
            ) == BookFileStatus::NotFound
        );

        ReaderCheckpoint removed_checkpoint;
        assert(
            checkpoint.load(
                record.book_id,
                removed_checkpoint
            ) == PersistStatus::NotFound
        );

        AppRestoreContext safe;
        assert(context.load(safe) == PersistStatus::Ok);
        assert(safe.screen == Screen::Library);
        assert(!safe.current_book.has_value());

        std::filesystem::remove_all(root, ec);
    }

    // Source deletion failure happens after Library removal; rollback restores
    // both the Library entry and the original Reading safe-context.
    {
        MemoryStateFileStore state_files;
        MemoryBookFileStore book_files;

        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        CborReaderCheckpointService checkpoint(state_files);
        CborAppContextService context(state_files);

        const auto record = makeBook(
            "source-failure",
            "/books/source-failure.txt"
        );

        assert(library.upsert(record) == LibraryStatus::Ok);
        assert(
            book_files.write(
                record.source_path,
                "Original bytes"
            ) == BookFileStatus::Ok
        );
        seedCheckpoint(checkpoint, record.book_id);

        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    record.book_id,
                }
            ) == PersistStatus::Ok
        );

        book_files.fail_remove = true;

        BookDeleteService deleter(
            library,
            book_files,
            checkpoint,
            context
        );

        assert(
            deleter.remove(record.book_id) ==
            BookDeleteStatus::SourceRemoveFailed
        );

        assert(library.get(record.book_id).has_value());

        std::string bytes;
        assert(
            book_files.read(
                record.source_path,
                bytes
            ) == BookFileStatus::Ok
        );
        assert(bytes == "Original bytes");

        AppRestoreContext restored;
        assert(context.load(restored) == PersistStatus::Ok);
        assert(restored.screen == Screen::Reading);
        assert(
            restored.current_book ==
            std::optional<BookId>{
                record.book_id
            }
        );
    }

    // Checkpoint deletion failure occurs after source removal. Rollback must
    // restore source, Library record, checkpoint and previous safe-context.
    {
        MemoryStateFileStore state_files;
        MemoryBookFileStore book_files;

        CborLibraryService library(state_files);
        assert(library.load() == LibraryStatus::Ok);

        CborReaderCheckpointService checkpoint(state_files);
        CborAppContextService context(state_files);

        const auto record = makeBook(
            "checkpoint-failure",
            "/books/checkpoint-failure.txt"
        );

        assert(library.upsert(record) == LibraryStatus::Ok);
        assert(
            book_files.write(
                record.source_path,
                "Rollback bytes"
            ) == BookFileStatus::Ok
        );
        seedCheckpoint(checkpoint, record.book_id);

        assert(
            context.save(
                AppRestoreContext{
                    Screen::Reading,
                    record.book_id,
                }
            ) == PersistStatus::Ok
        );

        state_files.fail_checkpoint_remove = true;

        BookDeleteService deleter(
            library,
            book_files,
            checkpoint,
            context
        );

        assert(
            deleter.remove(record.book_id) ==
            BookDeleteStatus::CheckpointRemoveFailed
        );

        assert(library.get(record.book_id).has_value());

        std::string bytes;
        assert(
            book_files.read(
                record.source_path,
                bytes
            ) == BookFileStatus::Ok
        );
        assert(bytes == "Rollback bytes");

        ReaderCheckpoint restored_checkpoint;
        assert(
            checkpoint.load(
                record.book_id,
                restored_checkpoint
            ) == PersistStatus::Ok
        );
        assert(
            restored_checkpoint.position.text_offset ==
            77
        );

        AppRestoreContext restored;
        assert(context.load(restored) == PersistStatus::Ok);
        assert(restored.screen == Screen::Reading);
        assert(
            restored.current_book ==
            std::optional<BookId>{
                record.book_id
            }
        );
    }

    return 0;
}
