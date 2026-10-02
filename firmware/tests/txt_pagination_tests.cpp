#include "enku/reader/pagination.hpp"
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
    TextPaginator paginator;

    ParserSourceInfo source{
        "book-page-test",
        "/books/paging.txt",
        "paging.txt",
    };

    const std::string text =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda.";

    const auto parsed = parser.parse(text, source);
    assert(parsed.ok());

    LayoutRequest request{
        "book-page-test",
        SemanticPosition{"book-page-test", "txt:body", 0},
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80},
    };

    const auto first = paginator.paginate(parsed.document, request, measurer);
    assert(first.ok());
    assert(!first.page.lines.empty());
    assert(first.page.lines.size() <= 3);
    assert(first.page.next_anchor.has_value());

    LayoutRequest second_request{
        "book-page-test",
        *first.page.next_anchor,
        request.typography,
        request.viewport,
    };

    const auto second =
        paginator.paginate(parsed.document, second_request, measurer);
    assert(second.ok());
    assert(!second.page.lines.empty());
    assert(
        second.page.first_position.text_offset >=
        first.page.last_position.text_offset
    );

    LayoutRequest bad_viewport = request;
    bad_viewport.viewport = Viewport{10, 10};

    const auto bad =
        paginator.paginate(parsed.document, bad_viewport, measurer);
    assert(bad.status == PaginationStatus::ViewportTooSmall);

    return 0;
}
