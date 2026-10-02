#include "enku/reader/document_reader_engine.hpp"
#include "enku/reader/reader_session.hpp"
#include "enku/reader/txt_parser.hpp"
#include "enku/runtime/reader_runtime.hpp"

#include <cassert>
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

} // namespace

int main() {
    TxtParser parser;
    FixedWidthMeasurer measurer;
    FakeRefreshService refresh;

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

    assert(session.open(request) == ReaderSessionStatus::Ready);

    AppState state;
    state.screen = Screen::Reading;
    state.current_book = BookId{"runtime-test"};
    state.reading_position = session.currentPage()->first_position;
    state.reading_progress = session.currentPage()->progress;

    ReaderRuntimeController runtime(state, session, refresh);

    const auto first_offset = state.reading_position->text_offset;

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(state.reading_position.has_value());
    assert(state.reading_position->text_offset > first_offset);
    assert(state.progress_dirty);
    assert(refresh.submitted == 1);
    assert(refresh.last.reason == RefreshReason::PageTurn);
    assert(refresh.last.refresh_class == RefreshClass::Full);
    assert(refresh.last.generation == 1);

    assert(
        runtime.handle(PagePreviousRequested{}) ==
        ReaderRuntimeResult::Applied
    );
    assert(state.reading_position->text_offset == first_offset);
    assert(refresh.submitted == 2);
    assert(refresh.last.generation == 2);

    state.screen = Screen::Library;
    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::Ignored
    );
    assert(refresh.submitted == 2);

    state.screen = Screen::Reading;
    refresh.accept = false;

    assert(
        runtime.handle(PageNextRequested{}) ==
        ReaderRuntimeResult::RefreshRejected
    );
    assert(state.reading_position.has_value());
    assert(state.reading_position->text_offset > first_offset);
    assert(state.progress_dirty);
    assert(refresh.submitted == 3);

    return 0;
}
