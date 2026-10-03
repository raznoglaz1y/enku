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

class FakePageRenderer final : public ReaderPageRenderer {
public:
    bool renderPage(
        const PageResult& page,
        const TypographySettings&,
        Orientation orientation
    ) override {
        last_orientation = orientation;
        ++calls;
        last_line_count =
            static_cast<std::uint32_t>(
                page.lines.size()
            );
        return accept;
    }

    bool accept{true};
    std::uint32_t calls{0};
    std::uint32_t last_line_count{0};
    Orientation last_orientation{Orientation::Landscape};
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
    LibraryStatus upsert(const BookRecord& value) override {
        record = value;
        return LibraryStatus::Ok;
    }

    LibraryStatus remove(const BookId& book_id) override {
        if (record.has_value() &&
            record->book_id == book_id) {
            record.reset();
            return LibraryStatus::Ok;
        }
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

    BookSourceStatus sourceSize(
        const BookRecord&,
        std::uint64_t& size_bytes
    ) override {
        ++size_calls;

        if (status != BookSourceStatus::Ok) {
            size_bytes = 0;
            return status;
        }

        size_bytes =
            static_cast<std::uint64_t>(
                content.size()
            );
        return BookSourceStatus::Ok;
    }

    BookSourceStatus readSourceRange(
        const BookRecord&,
        std::uint64_t offset,
        std::size_t length,
        std::string& bytes
    ) override {
        ++range_calls;

        if (status != BookSourceStatus::Ok) {
            bytes.clear();
            return status;
        }

        if (offset >
                static_cast<std::uint64_t>(
                    content.size()
                ) ||
            static_cast<std::uint64_t>(
                length
            ) >
                static_cast<std::uint64_t>(
                    content.size()
                ) - offset) {
            bytes.clear();
            return BookSourceStatus::ReadFailed;
        }

        bytes.assign(
            content,
            static_cast<std::size_t>(
                offset
            ),
            length
        );
        return BookSourceStatus::Ok;
    }

    BookSourceStatus status{BookSourceStatus::Ok};
    std::string content;
    std::uint32_t calls{0};
    std::uint32_t size_calls{0};
    std::uint32_t range_calls{0};
};

class FakeAppContextService final : public AppContextService {
public:
    PersistStatus load(
        AppRestoreContext& context
    ) override {
        if (!saved.has_value()) {
            return PersistStatus::NotFound;
        }
        context = *saved;
        return PersistStatus::Ok;
    }

    PersistStatus save(
        const AppRestoreContext& context
    ) override {
        ++saves;
        if (status == PersistStatus::Ok) {
            saved = context;
        }
        return status;
    }

    PersistStatus status{PersistStatus::Ok};
    std::optional<AppRestoreContext> saved;
    std::uint32_t saves{0};
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
    FakePageRenderer renderer;
    FakeLibraryService library;
    FakeBookSourceService source;
    FakeCheckpointService checkpoint;
    FakeAppContextService context;

    std::string text;
    for (std::uint32_t i = 0; i < 24U; ++i) {
        text +=
            "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
            "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega. ";
    }

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
        context,
        typography,
        viewport,
        &renderer
    );

    // OpenBookRequested now performs the loader step itself and immediately
    // completes through the BookOpened lifecycle on success.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );
    assert(source.calls == 0);
    assert(source.size_calls == 1);
    assert(source.range_calls > 0);
    assert(loader.session() != nullptr);
    assert(loader.session()->isOpen());
    assert(state.screen == Screen::Reading);
    assert(state.current_book == "runtime-test");
    assert(state.reading_position.has_value());
    assert(!state.progress_dirty);
    assert(context.saved.has_value());
    assert(context.saved->screen == Screen::Reading);
    assert(context.saved->current_book == "runtime-test");
    assert(refresh.last.reason == RefreshReason::PageTurn);
    assert(renderer.calls == 1);
    assert(renderer.last_line_count > 0);
    assert(renderer.last_orientation == Orientation::Portrait);

    const auto first_offset = state.reading_position->text_offset;

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    const auto saved_offset = state.reading_position->text_offset;
    assert(saved_offset > first_offset);
    assert(state.progress_dirty);
    assert(refresh.last.reason == RefreshReason::PageTurn);
    assert(renderer.calls == 2);

    assert(
        runtime.handle(BackRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(checkpoint.calls == 1);
    assert(state.screen == Screen::Library);
    assert(state.library.focused_book == "runtime-test");
    assert(!state.current_book.has_value());
    assert(loader.session() == nullptr);
    assert(context.saved.has_value());
    assert(context.saved->screen == Screen::Library);
    assert(!context.saved->current_book.has_value());
    assert(refresh.last.reason == RefreshReason::ScreenChanged);

    // Reopen restores the semantic position persisted by BackRequested.
    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::Applied
    );
    assert(source.calls == 0);
    assert(source.size_calls == 2);
    assert(source.range_calls > 1);
    assert(checkpoint.loads >= 2);
    assert(state.screen == Screen::Reading);
    assert(state.reading_position.has_value());
    assert(state.reading_position->text_offset == saved_offset);

    const auto checkpoints_before_long_read =
        checkpoint.calls;

    while (true) {
        const auto refresh_before =
            refresh.submitted;

        const auto result =
            runtime.handle(PageNextRequested{});

        if (result == ReaderRuntimeResult::EndOfBook) {
            assert(
                refresh.submitted ==
                refresh_before
            );
            break;
        }

        assert(result == ReaderRuntimeResult::Applied);
    }

    assert(
        checkpoint.calls >
        checkpoints_before_long_read
    );
    assert(state.current_book_finished);
    assert(state.reading_progress == 1.0F);
    assert(state.progress_dirty);
    assert(library.record->reading_state == ReadingState::Finished);

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
    assert(source.calls == 0);
    assert(source.size_calls == 3);
    assert(state.screen == Screen::Library);
    assert(state.library.focused_book == "runtime-test");
    assert(!state.current_book.has_value());
    assert(loader.session() == nullptr);
    assert(refresh.last.reason == RefreshReason::ErrorRecovery);

    // A renderer failure is surfaced before a display refresh can claim that
    // the newly laid-out page became visible.
    source.status = BookSourceStatus::Ok;
    renderer.accept = false;

    assert(
        runtime.handle(OpenBookRequested{"runtime-test"}) ==
        ReaderRuntimeResult::RenderFailed
    );
    assert(state.screen == Screen::Reading);

    return 0;
}
