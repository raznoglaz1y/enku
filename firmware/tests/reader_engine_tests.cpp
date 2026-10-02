#include "enku/reader/document_reader_engine.hpp"
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
        "reader-engine-test",
        "/books/reader.txt",
        "reader.txt",
    };

    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda "
        "mu nu xi omicron pi rho sigma tau.";

    const auto parsed = parser.parse(text, source);
    assert(parsed.ok());

    DocumentReaderEngine engine(parsed.document, measurer);

    LayoutRequest request{
        "reader-engine-test",
        SemanticPosition{"reader-engine-test", "txt:body", 0},
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80},
    };

    const auto first = engine.layoutPage(request);
    assert(first.has_value());
    assert(!first->lines.empty());
    assert(first->next_anchor.has_value());
    assert(!first->previous_anchor.has_value());

    LayoutRequest next_request{
        "reader-engine-test",
        *first->next_anchor,
        request.typography,
        request.viewport,
    };

    const auto second = engine.layoutPage(next_request);
    assert(second.has_value());
    assert(!second->lines.empty());
    assert(
        second->first_position.text_offset >=
        first->last_position.text_offset
    );

    LayoutRequest wrong_book = request;
    wrong_book.book_id = "other-book";
    const auto invalid = engine.layoutPage(wrong_book);
    assert(!invalid.has_value());

    return 0;
}
