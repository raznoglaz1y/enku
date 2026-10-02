#include "enku/reader/txt_parser.hpp"

#include <cassert>
#include <string>

using namespace enku;

int main() {
    TxtParser parser;

    ParserSourceInfo source{
        "book-test",
        "/books/hello.txt",
        "hello.txt",
    };

    {
        const std::string text = "First line\r\nsecond line.\r\n\r\nSecond paragraph.";
        const auto result = parser.parse(text, source);

        assert(result.ok());
        assert(result.document.metadata.title == "hello");
        assert(result.document.metadata.author_display == "Unknown author");
        assert(result.document.sections.size() == 1);
        assert(result.document.sections[0].blocks.size() == 2);
        assert(result.document.sections[0].blocks[0].text == "First line second line.");
        assert(result.document.sections[0].blocks[1].text == "Second paragraph.");
    }

    {
        const std::string bom = "\xEF\xBB\xBFUTF-8 BOM";
        const auto result = parser.parse(bom, source);
        assert(result.ok());
        assert(result.document.sections[0].blocks[0].text == "UTF-8 BOM");
    }

    {
        const std::string utf16_bom = "\xFF\xFEA\0";
        const auto result = parser.parse(utf16_bom, source);
        assert(result.status == ParserStatus::UnsupportedEncoding);
    }

    {
        const std::string invalid = "\xC0\xAF";
        const auto result = parser.parse(invalid, source);
        assert(result.status == ParserStatus::InvalidUtf8);
    }

    return 0;
}
