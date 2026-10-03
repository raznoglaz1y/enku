#include "enku/reader/document_reader_engine.hpp"
#include "enku/reader/txt_parser.hpp"

#include <cassert>
#include <string>
#include <string_view>
#include <utility>

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

    BookDocument multi;
    multi.book_id = "multi-section";
    multi.total_text_length = 52U;

    DocumentSection first_section;
    first_section.id = "chapter-1";
    first_section.text_length = 26U;
    first_section.blocks.push_back(
        TextBlock{
            TextBlockType::Paragraph,
            "Alpha beta gamma delta.",
            0U,
        }
    );

    DocumentSection second_section;
    second_section.id = "chapter-2";
    second_section.text_length = 26U;
    second_section.blocks.push_back(
        TextBlock{
            TextBlockType::Paragraph,
            "Epsilon zeta eta theta.",
            26U,
        }
    );

    multi.sections.push_back(
        std::move(first_section)
    );
    multi.sections.push_back(
        std::move(second_section)
    );

    DocumentReaderEngine multi_engine(
        multi,
        measurer
    );

    LayoutRequest multi_request{
        "multi-section",
        SemanticPosition{
            "multi-section",
            "chapter-1",
            0U,
        },
        TypographySettings{16, 1.0F, 10},
        Viewport{120, 60},
    };

    const auto multi_first =
        multi_engine.layoutPage(
            multi_request
        );

    assert(multi_first.has_value());
    assert(
        multi_first->first_position.section_id ==
        "chapter-1"
    );
    assert(multi_first->next_anchor.has_value());

    auto resume_request = multi_request;
    resume_request.anchor =
        *multi_first->next_anchor;

    const auto resumed =
        multi_engine.layoutPage(
            resume_request
        );

    assert(resumed.has_value());
    assert(
        resumed->first_position.book_id ==
        resume_request.anchor.book_id
    );
    assert(
        resumed->first_position.section_id ==
        resume_request.anchor.section_id
    );
    assert(
        resumed->first_position.text_offset >=
        resume_request.anchor.text_offset
    );

    auto section_resume = multi_request;
    section_resume.anchor =
        SemanticPosition{
            "multi-section",
            "chapter-2",
            26U,
        };

    const auto at_second =
        multi_engine.layoutPage(
            section_resume
        );

    assert(at_second.has_value());
    assert(
        at_second->first_position.section_id ==
        "chapter-2"
    );
    assert(
        at_second->first_position.text_offset ==
        26U
    );

    return 0;
}
