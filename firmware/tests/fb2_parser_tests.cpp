#include "enku/reader/fb2_parser.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

using namespace enku;

namespace {

class TrackingFb2RangeSource final
    : public Fb2RangeSource {
public:
    explicit TrackingFb2RangeSource(
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
        max_read =
            std::max(
                max_read,
                length
            );

        if (offset >
                static_cast<std::uint64_t>(
                    bytes_.size()
                ) ||
            static_cast<std::uint64_t>(
                length
            ) >
                static_cast<std::uint64_t>(
                    bytes_.size()
                ) -
                    offset) {
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
        return true;
    }

    mutable std::uint32_t reads{0};
    mutable std::size_t max_read{0};

private:
    std::string bytes_;
};

} // namespace

int main() {
    Fb2Parser parser;

    ParserSourceInfo source{
        "book-fb2-test",
        "/books/test.fb2",
        "test.fb2",
    };

    const std::string fb2 = R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info>
   <book-title>ENKU FB2</book-title>
   <author>
    <first-name>Alex</first-name>
    <last-name>Example</last-name>
   </author>
   <author>
    <nickname>Second Writer</nickname>
   </author>
   <lang>ru</lang>
   <annotation><p>FB2 parser fixture.</p></annotation>
  </title-info>
  <publish-info>
   <publisher>ENKU Project</publisher>
   <year>2026</year>
   <isbn>9780000000000</isbn>
  </publish-info>
 </description>
 <body>
  <section>
   <title><p>First Chapter</p></title>
   <p>First &#x2014; paragraph.</p>
   <cite><p>A cited sentence.</p></cite>
   <subtitle>Small heading</subtitle>
   <p>Second paragraph.</p>
  </section>
  <section>
   <title><p>Second Chapter</p></title>
   <p>Another chapter.</p>
  </section>
 </body>
</FictionBook>)";

    const auto metadata_only =
        parser.parseMetadata(
            fb2,
            source
        );

    assert(metadata_only.ok());
    assert(metadata_only.document.sections.empty());
    assert(
        metadata_only.document.metadata.title ==
        "ENKU FB2"
    );
    assert(
        metadata_only.document.metadata.authors.size() ==
        2
    );


    {
        const std::string mixed_case_metadata =
            R"(<?xml version="1.0" encoding="utf-8"?>
<FiCtIoNbOoK>
 <DeScRiPtIoN>
  <TiTlE-InFo>
   <BoOk-TiTlE>Mixed Case Metadata</BoOk-TiTlE>
   <AuThOr><NiCkNaMe>Case Author</NiCkNaMe></AuThOr>
   <LaNg>en</LaNg>
  </TiTlE-InFo>
 </DeScRiPtIoN>
 <BoDy>
  <SeCtIoN>
   <TiTlE><P>Chapter</P></TiTlE>
   <P>Body.</P>
  </SeCtIoN>
 </BoDy>
</FiCtIoNbOoK>)";

        TrackingFb2RangeSource ranged_metadata(
            mixed_case_metadata
        );

        const auto mixed =
            parser.parseMetadata(
                ranged_metadata,
                source
            );

        assert(mixed.ok());
        assert(
            mixed.document.metadata.title ==
            "Mixed Case Metadata"
        );
        assert(
            mixed.document.metadata.author_display ==
            "Case Author"
        );
        assert(
            mixed.document.metadata.authors.size() ==
            1U
        );
        assert(
            mixed.document.metadata.language ==
            std::optional<std::string>{"en"}
        );
        assert(
            mixed.document.metadata.toc_available
        );
        assert(ranged_metadata.reads > 0U);
        assert(
            ranged_metadata.max_read <=
            32U * 1024U
        );
    }

    const auto result =
        parser.parse(fb2, source);

    assert(result.ok());
    assert(
        result.document.metadata.title ==
        "ENKU FB2"
    );
    assert(
        result.document.metadata.author_display ==
        "Alex Example, Second Writer"
    );
    assert(
        result.document.metadata.authors.size() ==
        2
    );
    assert(
        result.document.metadata.language ==
        std::optional<std::string>{"ru"}
    );
    assert(
        result.document.metadata.publisher ==
        std::optional<std::string>{
            "ENKU Project"
        }
    );
    assert(
        result.document.metadata.published_date ==
        std::optional<std::string>{"2026"}
    );
    assert(
        result.document.metadata.identifier ==
        std::optional<std::string>{
            "9780000000000"
        }
    );
    assert(
        result.document.metadata.toc_available
    );

    assert(result.document.sections.size() == 2);
    assert(
        result.document.sections[0].title ==
        std::optional<std::string>{
            "First Chapter"
        }
    );
    assert(
        result.document.sections[1].title ==
        std::optional<std::string>{
            "Second Chapter"
        }
    );
    assert(
        result.document.sections[0].blocks.size() ==
        4
    );
    assert(
        result.document.sections[0].
            blocks[0].type ==
        TextBlockType::Paragraph
    );
    assert(
        result.document.sections[0].
            blocks[0].text ==
        "First — paragraph."
    );
    assert(
        result.document.sections[0].
            blocks[1].type ==
        TextBlockType::Quote
    );
    assert(
        result.document.sections[0].
            blocks[2].type ==
        TextBlockType::Heading
    );

    const std::string nested_fb2 = R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info>
   <book-title>Nested FB2</book-title>
  </title-info>
 </description>
 <body>
  <section>
   <title><p>Part One</p></title>
   <section>
    <title><p>Nested Chapter</p></title>
    <p>Nested body.</p>
   </section>
  </section>
 </body>
</FictionBook>)";

    const auto nested =
        parser.parse(
            nested_fb2,
            source
        );

    assert(nested.ok());
    assert(nested.document.sections.size() == 1);
    assert(
        nested.document.sections[0].title ==
        std::optional<std::string>{
            "Nested Chapter"
        }
    );
    assert(
        nested.document.sections[0].
            blocks[0].text ==
        "Nested body."
    );


    {
        const std::string mixed_nested_fb2 =
            R"(<?xml version="1.0" encoding="utf-8"?>
<FiCtIoNbOoK>
 <DeScRiPtIoN>
  <TiTlE-InFo>
   <BoOk-TiTlE>Mixed Nested FB2</BoOk-TiTlE>
  </TiTlE-InFo>
 </DeScRiPtIoN>
 <BoDy>
  <SeCtIoN>
   <TiTlE><P>Container</P></TiTlE>
   <SeCtIoN>
    <TiTlE><P>Mixed Leaf</P></TiTlE>
    <P>Mixed body.</P>
   </SeCtIoN>
  </SeCtIoN>
 </BoDy>
</FiCtIoNbOoK>)";

        const auto mixed_nested =
            parser.parse(
                mixed_nested_fb2,
                source
            );

        assert(mixed_nested.ok());
        assert(
            mixed_nested.document.sections.size() ==
            1U
        );
        assert(
            mixed_nested.document.sections[0].title ==
            std::optional<std::string>{
                "Mixed Leaf"
            }
        );
        assert(
            mixed_nested.document.sections[0].
                blocks.size() ==
            1U
        );
        assert(
            mixed_nested.document.sections[0].
                blocks[0].text ==
            "Mixed body."
        );
    }

    {
        std::string large_fb2 =
            R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info>
   <book-title>Ranged FB2</book-title>
   <author><nickname>Chunk Author</nickname></author>
  </title-info>
 </description>
 <body>
  <section>
   <title><p>Part</p></title>
   <section>
    <TITLE><P>Chunked Chapter</P></TITLE>
    <P class="body">)";

        large_fb2 +=
            std::string(
                96U * 1024U,
                'Z'
            );

        large_fb2 +=
            R"(</P>
   </section>
  </section>
  <section>
   <title><p>Second Leaf</p></title>
   <p>Second body.</p>
  </section>
 </body>
</FictionBook>)";

        TrackingFb2RangeSource ranged(
            std::move(large_fb2)
        );

        const auto ranged_result =
            parser.parse(
                ranged,
                source
            );

        assert(ranged_result.ok());
        assert(ranged.reads > 1U);
        assert(
            ranged.max_read <=
            32U * 1024U
        );
        assert(
            ranged_result.document.metadata.title ==
            "Ranged FB2"
        );
        assert(
            ranged_result.document.sections.size() ==
            2U
        );
        assert(
            ranged_result.document.sections[0].title ==
            std::optional<std::string>{
                "Chunked Chapter"
            }
        );
        assert(
            ranged_result.document.sections[0].
                blocks[0].text.size() ==
            96U * 1024U
        );
        assert(
            ranged_result.document.sections[1].title ==
            std::optional<std::string>{
                "Second Leaf"
            }
        );
    }

    {
        std::string oversized =
            R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info><book-title>Oversized Leaf</book-title></title-info>
 </description>
 <body>
  <section><p>)";

        oversized +=
            std::string(
                4U * 1024U * 1024U + 1024U,
                'Q'
            );

        oversized +=
            R"(</p></section>
 </body>
</FictionBook>)";

        TrackingFb2RangeSource ranged(
            std::move(oversized)
        );

        const auto limited =
            parser.parse(
                ranged,
                source
            );

        assert(
            limited.status ==
            ParserStatus::InvalidSource
        );
        assert(
            ranged.max_read <=
            32U * 1024U
        );
    }

    const auto unsupported_encoding =
        parser.parse(
            "<?xml version=\"1.0\" encoding=\"windows-1251\"?><FictionBook/>",
            source
        );

    assert(
        unsupported_encoding.status ==
        ParserStatus::UnsupportedEncoding
    );

    const auto invalid =
        parser.parse(
            "<xml/>",
            source
        );

    assert(
        invalid.status ==
        ParserStatus::InvalidSource
    );

    return 0;
}
