#include "enku/reader/pagination.hpp"

#include <cassert>
#include <string_view>

using namespace enku;

namespace {

class FixedWidthMeasurer final : public TextMeasurer {
public:
    std::uint16_t measureWidthPx(
        std::string_view utf8,
        const TypographySettings&
    ) const override {
        return static_cast<std::uint16_t>(
            utf8.size() * 8U
        );
    }

    std::uint16_t lineHeightPx(
        const TypographySettings&
    ) const override {
        return 20;
    }
};

} // namespace

int main() {
    BookDocument document;
    document.book_id = "semantic-book";

    DocumentSection section;
    section.id = "chapter-1";

    std::uint64_t offset = 0;

    auto add =
        [&](TextBlockType type,
            const char* text) mutable {
            TextBlock block;
            block.type = type;
            block.text = text;
            block.text_offset = offset;
            offset += block.text.size() + 1U;
            section.blocks.push_back(
                std::move(block)
            );
        };

    add(
        TextBlockType::Paragraph,
        "Opening paragraph."
    );
    add(
        TextBlockType::Heading,
        "Chapter Heading"
    );
    add(
        TextBlockType::Quote,
        "Quoted words."
    );
    add(
        TextBlockType::ListItem,
        "List entry."
    );

    section.text_length = offset;
    document.total_text_length = offset;
    document.sections.push_back(
        std::move(section)
    );

    FixedWidthMeasurer measurer;
    TextPaginator paginator;

    LayoutRequest request{
        "semantic-book",
        SemanticPosition{
            "semantic-book",
            "chapter-1",
            0,
        },
        TypographySettings{
            16,
            1.0F,
            10,
        },
        Viewport{
            320,
            180,
        },
    };

    const auto result =
        paginator.paginate(
            document,
            request,
            measurer
        );

    assert(result.ok());
    assert(result.page.lines.size() == 4);

    const auto& paragraph =
        result.page.lines[0];
    const auto& heading =
        result.page.lines[1];
    const auto& quote =
        result.page.lines[2];
    const auto& list =
        result.page.lines[3];

    assert(
        paragraph.kind ==
        PageLineKind::Paragraph
    );
    assert(
        heading.kind ==
        PageLineKind::Heading
    );
    assert(
        quote.kind ==
        PageLineKind::Quote
    );
    assert(
        list.kind ==
        PageLineKind::ListItem
    );

    assert(paragraph.x == 10U);
    assert(heading.x == 10U);
    assert(quote.x == 28U);
    assert(list.x == 32U);

    assert(paragraph.y == 10U);

    // One blank line is reserved before a heading that doesn't start a page.
    assert(heading.y == 50U);
    assert(quote.y == 70U);
    assert(list.y == 90U);

    return 0;
}
