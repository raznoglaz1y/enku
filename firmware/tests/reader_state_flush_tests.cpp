#include "enku/runtime/reader_state_flush.hpp"

#include <cassert>
#include <optional>

using namespace enku;

namespace {

class FakeLibrary final : public LibraryService {
public:
    LibraryStatus upsert(const BookRecord& value) override {
        record = value;
        return LibraryStatus::Ok;
    }

    LibraryStatus remove(const BookId&) override {
        return LibraryStatus::NotFound;
    }

    std::optional<BookRecord> get(
        const BookId& book_id
    ) const override {
        if (!record.has_value() ||
            record->book_id != book_id) {
            return std::nullopt;
        }
        return record;
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
        const BookId& book_id,
        ReadingState reading_state,
        float progress,
        std::uint64_t
    ) override {
        ++updates;
        if (!record.has_value() ||
            record->book_id != book_id) {
            return LibraryStatus::NotFound;
        }
        record->reading_state = reading_state;
        record->progress = progress;
        return update_status;
    }

    mutable std::optional<BookRecord> record;
    LibraryStatus update_status{LibraryStatus::Ok};
    std::uint32_t updates{0};
};

class FakeCheckpoint final : public ReaderCheckpointService {
public:
    PersistStatus load(
        const BookId&,
        ReaderCheckpoint&
    ) override {
        return PersistStatus::NotFound;
    }

    PersistStatus checkpoint(
        const BookId& book_id,
        const SemanticPosition& position,
        float progress,
        ReadingState reading_state
    ) override {
        ++writes;
        last_book = book_id;
        last_position = position;
        last_progress = progress;
        last_state = reading_state;
        return status;
    }

    PersistStatus status{PersistStatus::Ok};
    std::uint32_t writes{0};
    BookId last_book;
    SemanticPosition last_position;
    float last_progress{0.0F};
    ReadingState last_state{ReadingState::New};
};

class FakeContext final : public AppContextService {
public:
    PersistStatus load(
        AppRestoreContext&
    ) override {
        return PersistStatus::NotFound;
    }

    PersistStatus save(
        const AppRestoreContext& context
    ) override {
        ++saves;
        last = context;
        return status;
    }

    PersistStatus status{PersistStatus::Ok};
    std::uint32_t saves{0};
    AppRestoreContext last;
};

} // namespace

int main() {
    AppState app;
    app.screen = Screen::Reading;
    app.current_book = BookId{"flush-book"};
    app.reading_position =
        SemanticPosition{
            "flush-book",
            "chapter-1",
            120U,
        };
    app.reading_progress = 0.35F;
    app.progress_dirty = true;

    FakeLibrary library;
    BookRecord record;
    record.book_id = "flush-book";
    record.last_opened_order = 7U;
    library.record = record;

    FakeCheckpoint checkpoint;
    FakeContext context;

    ReaderStateFlushCoordinator flush(
        app,
        library,
        checkpoint,
        context
    );

    assert(
        flush.flush(
            ReaderStateFlushTarget::PreserveReading
        ) == ReaderStateFlushStatus::Applied
    );
    assert(checkpoint.writes == 1U);
    assert(!app.progress_dirty);
    assert(library.updates == 1U);
    assert(context.saves == 1U);
    assert(context.last.screen == Screen::Reading);
    assert(
        context.last.current_book ==
        std::optional<BookId>{"flush-book"}
    );

    // A clean flush still refreshes summary/context, but must not write
    // another checkpoint to storage.
    assert(
        flush.flush(
            ReaderStateFlushTarget::PreserveReading
        ) == ReaderStateFlushStatus::Applied
    );
    assert(checkpoint.writes == 1U);
    assert(library.updates == 2U);
    assert(context.saves == 2U);

    app.progress_dirty = true;
    app.reading_position->text_offset = 240U;
    app.reading_progress = 0.6F;

    assert(
        flush.flush(
            ReaderStateFlushTarget::ReturnToLibrary
        ) == ReaderStateFlushStatus::Applied
    );
    assert(checkpoint.writes == 2U);
    assert(!app.progress_dirty);
    assert(context.last.screen == Screen::Library);
    assert(!context.last.current_book.has_value());

    // A stale current_book must not turn Library into a reading restore
    // context during sleep/power-off style flushes.
    app.screen = Screen::Library;
    app.progress_dirty = true;
    const auto writes_before_library =
        checkpoint.writes;

    assert(
        flush.flush(
            ReaderStateFlushTarget::PreserveReading
        ) == ReaderStateFlushStatus::Applied
    );
    assert(
        checkpoint.writes ==
        writes_before_library
    );
    assert(context.last.screen == Screen::Library);
    assert(!context.last.current_book.has_value());

    checkpoint.status = PersistStatus::IoError;
    app.screen = Screen::Reading;
    app.progress_dirty = true;

    assert(
        flush.flush(
            ReaderStateFlushTarget::PreserveReading
        ) == ReaderStateFlushStatus::CheckpointFailed
    );
    assert(app.progress_dirty);

    return 0;
}
