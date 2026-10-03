#include "enku/reader/document_reader_engine.hpp"
#include "enku/reader/reader_session.hpp"
#include "enku/reader/txt_parser.hpp"

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

} // namespace

int main() {
    TxtParser parser;
    FixedWidthMeasurer measurer;

    ParserSourceInfo source{
        "session-test",
        "/books/session.txt",
        "session.txt",
    };

    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
        "mu nu xi omicron pi rho sigma tau upsilon phi chi psi omega.";

    const auto parsed = parser.parse(text, source);
    assert(parsed.ok());

    DocumentReaderEngine engine(parsed.document, measurer);
    ReaderSession session(engine);

    LayoutRequest request{
        "session-test",
        SemanticPosition{"session-test", "txt:body", 0},
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80},
    };

    assert(session.open(request) == ReaderSessionStatus::Ready);
    assert(session.currentPage().has_value());

    const auto first_start =
        session.currentPage()->first_position.text_offset;
    assert(!session.currentPage()->previous_anchor.has_value());
    assert(session.currentPage()->next_anchor.has_value());

    assert(session.next() == ReaderSessionStatus::Ready);
    assert(session.currentPage()->previous_anchor.has_value());

    const auto second_start =
        session.currentPage()->first_position.text_offset;
    assert(second_start > first_start);

    assert(session.previous() == ReaderSessionStatus::Ready);
    assert(
        session.currentPage()->first_position.text_offset ==
        first_start
    );

    assert(
        session.previous() ==
        ReaderSessionStatus::BeginningOfBook
    );
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.text_offset ==
        first_start
    );

    assert(session.next() == ReaderSessionStatus::Ready);
    assert(session.currentPage().has_value());

    const auto before_relayout =
        session.currentPage()->first_position;

    session.invalidateLayout(
        TypographySettings{18, 1.1F, 12},
        Viewport{160, 100}
    );

    assert(session.status() == ReaderSessionStatus::Ready);
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.book_id ==
        before_relayout.book_id
    );
    assert(
        session.currentPage()->first_position.section_id ==
        before_relayout.section_id
    );
    assert(
        session.currentPage()->first_position.text_offset >=
        before_relayout.text_offset
    );

    const auto after_typography =
        session.currentPage()->first_position;

    session.invalidateLayout(
        TypographySettings{18, 1.1F, 12},
        Viewport{100, 160}
    );

    assert(session.status() == ReaderSessionStatus::Ready);
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.book_id ==
        after_typography.book_id
    );
    assert(
        session.currentPage()->first_position.section_id ==
        after_typography.section_id
    );
    assert(
        session.currentPage()->first_position.text_offset >=
        after_typography.text_offset
    );

    std::uint32_t forward_steps = 0;
    while (session.next() == ReaderSessionStatus::Ready) {
        ++forward_steps;
        assert(forward_steps < 128U);
    }

    assert(
        session.status() ==
        ReaderSessionStatus::EndOfBook
    );
    assert(session.currentPage().has_value());

    const auto end_position =
        session.currentPage()->first_position;

    assert(
        session.next() ==
        ReaderSessionStatus::EndOfBook
    );
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.book_id ==
        end_position.book_id
    );
    assert(
        session.currentPage()->first_position.section_id ==
        end_position.section_id
    );
    assert(
        session.currentPage()->first_position.text_offset ==
        end_position.text_offset
    );

    assert(
        session.previous() ==
        ReaderSessionStatus::Ready
    );
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.text_offset <
        end_position.text_offset
    );

    assert(
        session.next() ==
        ReaderSessionStatus::Ready
    );
    assert(session.currentPage().has_value());
    assert(
        session.currentPage()->first_position.text_offset ==
        end_position.text_offset
    );

    return 0;
}
