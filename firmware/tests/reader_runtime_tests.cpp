#include "enku/reader/document_reader_engine.hpp"
#include "enku/reader/reader_session.hpp"
#include "enku/reader/txt_parser.hpp"
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

class FakeCheckpointService final : public ReaderCheckpointService {
public:
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
        return status;
    }

    PersistStatus status{PersistStatus::Ok};
    std::uint32_t calls{0};
    BookId last_book;
    SemanticPosition last_position;
    float last_progress{0.0F};
    ReadingState last_state{ReadingState::New};
};

} // namespace

int main() {
    TxtParser parser;
    FixedWidthMeasurer measurer;
    FakeRefreshService refresh;
    FakeLibraryService library;
    FakeCheckpointService checkpoint;

    ParserSourceInfo source{
        "runtime-test",
        "/books/runtime.txt",
        "runtime.txt",
    };

    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
        "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega.";

    const auto parsed = parser.parse(text, source);
    assert(parsed.ok());

    DocumentReaderEngine engine(parsed.document, measurer);
    ReaderSession session(engine);

    LayoutRequest request{
        "runtime-test",
        SemanticPosition{"runtime-test", "txt:body", 0},
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80},
    };

    BookRecord record;
    record.book_id = "runtime-test";
    record.format = BookFormat::Txt;
    record.metadata.title = "runtime";
    record.last_opened_order = 10;
    library.record = record;

    AppState state;
    state.screen = Screen::Library;

    ReaderRuntimeController runtime(
        state,
        session,
        refresh,
        library,
        checkpoint
    );

    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::BookOpening
    );
    assert(state.screen == Screen::BookOpening);
    assert(state.current_book == "runtime-test");
    assert(refresh.last.reason == RefreshReason::ScreenChanged);

    // Simulates the book-loader layer completing parse/open before BookOpened.
    assert(session.open(request) == ReaderSessionStatus::Ready);

    assert(
        runtime.handle(BookOpened{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );
    assert(state.screen == Screen::Reading);
    assert(state.reading_position.has_value());
    assert(!state.progress_dirty);

    const auto first_offset = state.reading_position->text_offset;

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(state.reading_position->text_offset > first_offset);
    assert(state.progress_dirty);
    assert(refresh.last.reason == RefreshReason::PageTurn);

    assert(
        runtime.handle(PagePreviousRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(state.reading_position->text_offset == first_offset);

    assert(
        runtime.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(checkpoint.calls == 1);
    assert(state.screen == Screen::Library);
    assert(state.library.focused_book == "runtime-test");
    assert(!state.current_book.has_value());
    assert(!session.isOpen());
    assert(refresh.last.reason == RefreshReason::ScreenChanged);

    // Open again and advance to end.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::BookOpening
    );
    assert(session.open(request) == ReaderSessionStatus::Ready);
    assert(
        runtime.handle(BookOpened{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );

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

    // Book-open failure returns safely to Library.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::BookOpening
    );
    assert(
        runtime.handle(BookOpenFailed{"runtime-test"}) ==
        ReaderRuntimeResult::BookOpenFailed
    );
    assert(state.screen == Screen::Library);
    assert(!state.current_book.has_value());
    assert(refresh.last.reason == RefreshReason::ErrorRecovery);

    return 0;
}
