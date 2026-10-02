#include "enku/reader/book_loader.hpp"
#include "enku/runtime/reader_runtime.hpp"

#include <cassert>
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
    bool busy() const override {
        return false;
    }

    bool submit(const RefreshRequest& request) override {
        ++submitted;
        last = request;
        return accept;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }

    bool accept{true};
    std::uint32_t submitted{0};
    RefreshRequest last;
};

class FakeLibraryService final : public LibraryService {
public:
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
        std::uint64_t last_opened_order
    ) override {
        ++updates;
        if (!record.has_value() ||
            record->book_id != book_id) {
            return LibraryStatus::NotFound;
        }

        record->reading_state = reading_state;
        record->progress = progress;
        record->last_opened_order = last_opened_order;
        return update_status;
    }

    mutable std::optional<BookRecord> record;
    LibraryStatus update_status{LibraryStatus::Ok};
    std::uint32_t updates{0};
};

class FakeBookSourceService final : public BookSourceService {
public:
    BookSourceStatus readSource(
        const BookRecord&,
        std::string& bytes
    ) override {
        ++calls;
        bytes = content;
        return status;
    }

    BookSourceStatus status{BookSourceStatus::Ok};
    std::string content;
    std::uint32_t calls{0};
};

class FakeCheckpointService final : public ReaderCheckpointService {
public:
    PersistStatus load(
        const BookId& book_id,
        ReaderCheckpoint& checkpoint
    ) override {
        ++loads;

        if (load_status != PersistStatus::Ok) {
            return load_status;
        }

        if (!saved.has_value() ||
            saved->position.book_id != book_id) {
            return PersistStatus::NotFound;
        }

        checkpoint = *saved;
        return PersistStatus::Ok;
    }

    PersistStatus checkpoint(
        const BookId& book_id,
        const SemanticPosition& position,
        float progress,
        ReadingState reading_state
    ) override {
        ++calls;
        last_book = book_id;
        last_position = position;
        last_progress = progress;
        last_state = reading_state;
        if (status == PersistStatus::Ok) {
            saved = ReaderCheckpoint{
                position,
                progress,
                reading_state,
            };
        }
        return status;
    }

    PersistStatus status{PersistStatus::Ok};
    PersistStatus load_status{PersistStatus::Ok};
    std::optional<ReaderCheckpoint> saved;
    std::uint32_t loads{0};
    std::uint32_t calls{0};
    BookId last_book;
    SemanticPosition last_position;
    float last_progress{0.0F};
    ReadingState last_state{ReadingState::New};
};

} // namespace

int main() {
    FixedWidthMeasurer measurer;
    FakeRefreshService refresh;
    FakeLibraryService library;
    FakeBookSourceService source;
    FakeCheckpointService checkpoint;

    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
        "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega.";

    BookRecord record;
    record.book_id = "runtime-test";
    record.format = BookFormat::Txt;
    record.source_path = "/books/runtime.txt";
    record.source_filename = "runtime.txt";
    record.metadata.title = "runtime";
    record.last_opened_order = 10;
    library.record = record;
    source.content = text;

    ReaderBookLoader loader(library, source, measurer);

    AppState state;
    state.screen = Screen::Library;

    const TypographySettings typography{16, 1.0F, 10};
    const Viewport viewport{140, 80};

    ReaderRuntimeController runtime(
        state,
        loader,
        refresh,
        library,
        checkpoint,
        typography,
        viewport
    );

    // OpenBookRequested now performs the loader step itself and immediately
    // completes through the BookOpened lifecycle on success.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );
    assert(source.calls == 1);
    assert(loader.session() != nullptr);
    assert(loader.session()->isOpen());
    assert(state.screen == Screen::Reading);
    assert(state.current_book == "runtime-test");
    assert(state.reading_position.has_value());
    assert(!state.progress_dirty);
    assert(refresh.last.reason == RefreshReason::PageTurn);

    const auto first_offset = state.reading_position->text_offset;

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    const auto saved_offset = state.reading_position->text_offset;
    assert(saved_offset > first_offset);
    assert(state.progress_dirty);
    assert(refresh.last.reason == RefreshReason::PageTurn);

    assert(
        runtime.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(checkpoint.calls == 1);
    assert(state.screen == Screen::Library);
    assert(state.library.focused_book == "runtime-test");
    assert(!state.current_book.has_value());
    assert(loader.session() == nullptr);
    assert(refresh.last.reason == RefreshReason::ScreenChanged);

    // Reopen restores the semantic position persisted by BackRequested.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );
    assert(source.calls == 2);
    assert(checkpoint.loads >= 2);
    assert(state.screen == Screen::Reading);
    assert(state.reading_position.has_value());
    assert(state.reading_position->text_offset == saved_offset);

    while (true) {
        const auto result =
            runtime.handle(PageNextRequested{});

        if (result == ReaderRuntimeResult::EndOfBook) {
            break;
        }

        assert(result == ReaderRuntimeResult::Applied);
    }

    assert(state.current_book_finished);
    assert(state.reading_progress == 1.0F);
    assert(state.progress_dirty);
    assert(library.record->reading_state == ReadingState::Finished);
    assert(refresh.last.reason == RefreshReason::StatusChanged);

    assert(
        runtime.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(checkpoint.last_state == ReadingState::Finished);
    assert(state.screen == Screen::Library);

    // Loader failure automatically completes through BookOpenFailed and
    // returns safely to Library.
    source.status = BookSourceStatus::Unavailable;

    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::BookOpenFailed
    );
    assert(source.calls == 3);
    assert(state.screen == Screen::Library);
    assert(state.library.focused_book == "runtime-test");
    assert(!state.current_book.has_value());
    assert(loader.session() == nullptr);
    assert(refresh.last.reason == RefreshReason::ErrorRecovery);

    return 0;
}
