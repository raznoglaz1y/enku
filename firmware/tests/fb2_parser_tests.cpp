#include "enku/reader/fb2_parser.hpp"

#include <cassert>
#include <optional>
#include <string>

using namespace enku;

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
