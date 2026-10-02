#include "enku/reader/epub_parser.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace enku;

namespace {

void u16(
    std::string& out,
    std::uint16_t value
) {
    out.push_back(
        static_cast<char>(value & 0xFFU)
    );
    out.push_back(
        static_cast<char>(
            (value >> 8U) & 0xFFU
        )
    );
}

void u32(
    std::string& out,
    std::uint32_t value
) {
    u16(
        out,
        static_cast<std::uint16_t>(
            value & 0xFFFFU
        )
    );
    u16(
        out,
        static_cast<std::uint16_t>(
            value >> 16U
        )
    );
}

struct StoredEntry {
    std::string name;
    std::string bytes;
    std::uint32_t local_offset{0};
};

std::string makeStoredZip(
    std::vector<StoredEntry> entries
) {
    std::string out;

    for (auto& entry : entries) {
        entry.local_offset =
            static_cast<std::uint32_t>(
                out.size()
            );

        u32(out, 0x04034B50U);
        u16(out, 20);
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u32(out, 0);
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        u16(
            out,
            static_cast<std::uint16_t>(
                entry.name.size()
            )
        );
        u16(out, 0);
        out += entry.name;
        out += entry.bytes;
    }

    const auto central_offset =
        static_cast<std::uint32_t>(
            out.size()
        );

    for (const auto& entry : entries) {
        u32(out, 0x02014B50U);
        u16(out, 20);
        u16(out, 20);
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u32(out, 0);
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        u16(
            out,
            static_cast<std::uint16_t>(
                entry.name.size()
            )
        );
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u16(out, 0);
        u32(out, 0);
        u32(out, entry.local_offset);
        out += entry.name;
    }

    const auto central_size =
        static_cast<std::uint32_t>(
            out.size()
        ) - central_offset;

    u32(out, 0x06054B50U);
    u16(out, 0);
    u16(out, 0);
    u16(
        out,
        static_cast<std::uint16_t>(
            entries.size()
        )
    );
    u16(
        out,
        static_cast<std::uint16_t>(
            entries.size()
        )
    );
    u32(out, central_size);
    u32(out, central_offset);
    u16(out, 0);

    return out;
}

std::string sampleEpub() {
    return makeStoredZip({
        {
            "mimetype",
            "application/epub+zip",
        },
        {
            "META-INF/container.xml",
            R"(<?xml version="1.0"?>
<container>
  <rootfiles>
    <rootfile full-path="OEBPS/content.opf"
      media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>)",
        },
        {
            "OEBPS/content.opf",
            R"(<?xml version="1.0" encoding="UTF-8"?>
<package version="3.0"
 xmlns:dc="http://purl.org/dc/elements/1.1/">
 <metadata>
  <dc:title>ENKU &amp; EPUB</dc:title>
  <dc:creator>Alex Example</dc:creator>
  <dc:language>en</dc:language>
  <dc:identifier>urn:enku:test</dc:identifier>
 </metadata>
 <manifest>
  <item id="nav" href="nav.xhtml"
   media-type="application/xhtml+xml"
   properties="nav"/>
  <item id="c1" href="Text/ch1.xhtml"
   media-type="application/xhtml+xml"/>
  <item id="c2" href="Text/ch2.xhtml"
   media-type="application/xhtml+xml"/>
 </manifest>
 <spine>
  <itemref idref="c1"/>
  <itemref idref="c2"/>
 </spine>
</package>)",
        },
        {
            "OEBPS/Text/ch1.xhtml",
            R"(<html><body>
<h1>Chapter One</h1>
<p>First &amp; important paragraph.</p>
<p>Second paragraph.</p>
</body></html>)",
        },
        {
            "OEBPS/Text/ch2.xhtml",
            R"(<html><body>
<p>Chapter two text.</p>
</body></html>)",
        },
    });
}

} // namespace

int main() {
    EpubParser parser;

    ParserSourceInfo source{
        "book-epub-test",
        "/books/test.epub",
        "test.epub",
    };

    const auto result =
        parser.parse(
            sampleEpub(),
            source
        );

    assert(result.ok());
    assert(
        result.document.book_id ==
        "book-epub-test"
    );
    assert(
        result.document.metadata.title ==
        "ENKU & EPUB"
    );
    assert(
        result.document.metadata.author_display ==
        "Alex Example"
    );
    assert(
        result.document.metadata.language ==
        std::optional<std::string>{"en"}
    );
    assert(
        result.document.metadata.identifier ==
        std::optional<std::string>{
            "urn:enku:test"
        }
    );
    assert(
        result.document.metadata.toc_available
    );
    assert(result.document.sections.size() == 2);
    assert(
        result.document.sections[0].id ==
        "OEBPS/Text/ch1.xhtml"
    );
    assert(
        result.document.sections[0].blocks.size() ==
        3
    );
    assert(
        result.document.sections[0].
            blocks[0].text ==
        "Chapter One"
    );
    assert(
        result.document.sections[0].
            blocks[1].text ==
        "First & important paragraph."
    );
    assert(
        result.document.sections[1].
            blocks[0].text ==
        "Chapter two text."
    );
    assert(
        result.document.total_text_length > 0
    );

    const auto invalid =
        parser.parse(
            "not a zip",
            source
        );

    assert(
        invalid.status ==
        ParserStatus::InvalidSource
    );

    return 0;
}
