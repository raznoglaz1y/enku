#include "enku/reader/txt_parser.hpp"

#include <cassert>
#include <cstdint>
#include <string>

using namespace enku;

namespace {

class StringTextRangeSource final
    : public TextRangeSource {
public:
    explicit StringTextRangeSource(
        std::string bytes
    )
        : bytes_(std::move(bytes)) {}

    std::uint64_t size() const override {
        return static_cast<std::uint64_t>(
            bytes_.size()
        );
    }

    bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const override {
        ++reads;

        if (offset >
                static_cast<std::uint64_t>(
                    bytes_.size()
                ) ||
            static_cast<std::uint64_t>(
                length
            ) >
                static_cast<std::uint64_t>(
                    bytes_.size()
                ) - offset) {
            out.clear();
            return false;
        }

        out.assign(
            bytes_,
            static_cast<std::size_t>(
                offset
            ),
            length
        );
        max_read =
            std::max(
                max_read,
                length
            );
        return true;
    }

    mutable std::uint32_t reads{0};
    mutable std::size_t max_read{0};

private:
    std::string bytes_;
};

} // namespace

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
        const std::string utf16_bom{
            static_cast<char>(0xFF),
            static_cast<char>(0xFE),
            'A',
            '\0',
        };
        const auto result = parser.parse(utf16_bom, source);
        assert(result.status == ParserStatus::UnsupportedEncoding);
    }

    {
        const std::string invalid = "\xC0\xAF";
        const auto result = parser.parse(invalid, source);
        assert(result.status == ParserStatus::InvalidUtf8);
    }

    {
        std::string large(
            32U * 1024U - 1U,
            'A'
        );

        large +=
            "\xE2\x80\x94";
        large +=
            "\r\ncontinued\r\n\r\nsecond";

        StringTextRangeSource ranged(
            large
        );

        const auto result =
            parser.parse(
                ranged,
                source
            );

        assert(result.ok());
        assert(ranged.reads >= 2U);
        assert(ranged.max_read <=
            32U * 1024U);
        assert(
            result.document.sections[0].
                blocks.size() ==
            2U
        );

        const auto& first =
            result.document.sections[0].
                blocks[0].text;

        assert(
            first.find("— continued") !=
            std::string::npos
        );
        assert(
            result.document.sections[0].
                blocks[1].text ==
            "second"
        );
    }

    return 0;
}
