#include "enku/reader/epub_parser.hpp"
#include "enku/reader/zip_archive.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <zlib.h>

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
    bool deflate{false};
    std::uint32_t local_offset{0};
    std::string compressed;
};

std::string rawDeflate(
    const std::string& bytes
) {
    z_stream stream = {};

    assert(
        deflateInit2(
            &stream,
            Z_BEST_SPEED,
            Z_DEFLATED,
            -MAX_WBITS,
            8,
            Z_DEFAULT_STRATEGY
        ) == Z_OK
    );

    std::string out;
    out.resize(
        compressBound(bytes.size())
    );

    stream.next_in =
        reinterpret_cast<Bytef*>(
            const_cast<char*>(bytes.data())
        );
    stream.avail_in =
        static_cast<uInt>(bytes.size());
    stream.next_out =
        reinterpret_cast<Bytef*>(out.data());
    stream.avail_out =
        static_cast<uInt>(out.size());

    assert(
        deflate(&stream, Z_FINISH) ==
        Z_STREAM_END
    );

    out.resize(stream.total_out);
    deflateEnd(&stream);
    return out;
}

std::string makeStoredZip(
    std::vector<StoredEntry> entries
) {
    std::string out;

    for (auto& entry : entries) {
        entry.local_offset =
            static_cast<std::uint32_t>(
                out.size()
            );
        entry.compressed =
            entry.deflate
                ? rawDeflate(entry.bytes)
                : entry.bytes;

        u32(out, 0x04034B50U);
        u16(out, 20);
        u16(out, 0);
        u16(out, entry.deflate ? 8 : 0);
        u16(out, 0);
        u16(out, 0);
        u32(out, 0);
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.compressed.size()
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
        out += entry.compressed;
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
        u16(out, entry.deflate ? 8 : 0);
        u16(out, 0);
        u16(out, 0);
        u32(out, 0);
        u32(
            out,
            static_cast<std::uint32_t>(
                entry.compressed.size()
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

std::string oversizedDeclaredZip() {
    const std::string name = "huge.xhtml";
    std::string out;

    appendU32(out, 0x04034B50U);
    appendU16(out, 20U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU32(out, 0U);
    appendU32(out, 1U);
    appendU32(
        out,
        static_cast<std::uint32_t>(
            ZipArchive::kMaxEntryBytes + 1U
        )
    );
    appendU16(
        out,
        static_cast<std::uint16_t>(
            name.size()
        )
    );
    appendU16(out, 0U);
    out += name;
    out.push_back('x');

    const auto central_offset =
        static_cast<std::uint32_t>(
            out.size()
        );

    appendU32(out, 0x02014B50U);
    appendU16(out, 20U);
    appendU16(out, 20U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU32(out, 0U);
    appendU32(out, 1U);
    appendU32(
        out,
        static_cast<std::uint32_t>(
            ZipArchive::kMaxEntryBytes + 1U
        )
    );
    appendU16(
        out,
        static_cast<std::uint16_t>(
            name.size()
        )
    );
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU32(out, 0U);
    appendU32(out, 0U);
    out += name;

    const auto central_size =
        static_cast<std::uint32_t>(
            out.size()
        ) - central_offset;

    appendU32(out, 0x06054B50U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(out, 1U);
    appendU16(out, 1U);
    appendU32(out, central_size);
    appendU32(out, central_offset);
    appendU16(out, 0U);

    return out;
}

std::string sampleEpub() {
    return makeStoredZip({
        {
            "mimetype",
            "application/epub+zip",
            false,
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
            true,
        },
        {
            "OEBPS/content.opf",
            R"(<?xml version="1.0" encoding="UTF-8"?>
<package version="3.0"
 xmlns:dc="http://purl.org/dc/elements/1.1/">
 <metadata>
  <dc:title>ENKU &amp; EPUB</dc:title>
  <dc:creator>Alex Example</dc:creator>
  <dc:creator>Second Author</dc:creator>
  <dc:language>en</dc:language>
  <dc:description>Compact reader test book.</dc:description>
  <dc:publisher>ENKU Project</dc:publisher>
  <dc:date>2026-10-03</dc:date>
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
  <item id="notes" href="Text/notes.xhtml"
   media-type="application/xhtml+xml"/>
 </manifest>
 <spine>
  <itemref idref="c1"/>
  <itemref idref="c2"/>
  <itemref idref="notes" linear="no"/>
 </spine>
</package>)",
            true,
        },
        {
            "OEBPS/nav.xhtml",
            R"(<html><body>
<nav epub:type="toc">
<ol>
<li><a href="Text/ch1.xhtml">The First Chapter</a></li>
<li><a href="Text/ch2.xhtml#start">The Second Chapter</a></li>
</ol>
</nav>
</body></html>)",
            true,
        },
        {
            "OEBPS/Text/ch1.xhtml",
            R"(<html><body>
<h1>Chapter One</h1>
<p>First &#x2014; important<br/>paragraph.</p>
<blockquote>A short quotation.</blockquote>
<ul><li>A list item.</li></ul>
<p>Second paragraph.</p>
</body></html>)",
            true,
        },
        {
            "OEBPS/Text/notes.xhtml",
            R"(<html><body><p>Should not appear in reading spine.</p></body></html>)",
            true,
        },
        {
            "OEBPS/Text/ch2.xhtml",
            R"(<html><body>
<p>Chapter two text.</p>
</body></html>)",
            true,
        },
    });
}

std::string sampleEpub2() {
    return makeStoredZip({
        {
            "mimetype",
            "application/epub+zip",
            false,
        },
        {
            "META-INF/container.xml",
            R"(<?xml version="1.0"?>
<container>
  <rootfiles>
    <rootfile full-path="OPS/package.opf"
      media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>)",
            true,
        },
        {
            "OPS/package.opf",
            R"(<?xml version="1.0"?>
<package version="2.0"
 xmlns:dc="http://purl.org/dc/elements/1.1/">
 <metadata>
  <dc:title>Legacy EPUB</dc:title>
  <dc:creator>Legacy Author</dc:creator>
 </metadata>
 <manifest>
  <item id="ncx" href="toc.ncx"
   media-type="application/x-dtbncx+xml"/>
  <item id="legacy1" href="chapters/one.xhtml"
   media-type="application/xhtml+xml"/>
 </manifest>
 <spine toc="ncx">
  <itemref idref="legacy1"/>
 </spine>
</package>)",
            true,
        },
        {
            "OPS/toc.ncx",
            R"(<?xml version="1.0"?>
<ncx>
 <navMap>
  <navPoint id="n1">
   <navLabel><text>Legacy Chapter</text></navLabel>
   <content src="chapters/one.xhtml#p1"/>
  </navPoint>
 </navMap>
</ncx>)",
            true,
        },
        {
            "OPS/chapters/one.xhtml",
            R"(<html><body>
<h2>Fallback Heading</h2>
<p id="p1">Old EPUB text.</p>
</body></html>)",
            true,
        },
    });
}


std::string largePseudoRandomText(
    std::size_t length
) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";

    std::string out;
    out.reserve(length);

    std::uint32_t state =
        0x12345678U;

    for (std::size_t i = 0;
         i < length;
         ++i) {
        state =
            state * 1664525U +
            1013904223U;

        out.push_back(
            alphabet[
                (state >> 24U) & 0x3FU
            ]
        );
    }

    return out;
}

std::string sampleLargeRangedEpub() {
    const std::string chapter =
        "<HTML><Body><P class=\"main\">" +
        largePseudoRandomText(
            192U * 1024U
        ) +
        "</P></Body></HTML>";

    std::vector<StoredEntry> entries{
        {
            "META-INF/container.xml",
            R"(<container><rootfiles><rootfile full-path="OEBPS/content.opf"/></rootfiles></container>)",
            false,
        },
        {
            "OEBPS/content.opf",
            R"(<package xmlns:dc="http://purl.org/dc/elements/1.1/"><metadata><dc:title>Ranged EPUB</dc:title></metadata><manifest><item id="c1" href="ch1.xhtml" media-type="application/xhtml+xml"/></manifest><spine><itemref idref="c1"/></spine></package>)",
            false,
        },
        {
            "OEBPS/ch1.xhtml",
            chapter,
            true,
        },
        {
            "unused-large.bin",
            std::string(
                512U * 1024U,
                'x'
            ),
            false,
        },
    };

    return makeStoredZip(
        std::move(entries)
    );
}

class TrackingZipRangeSource final
    : public ZipRangeSource {
public:
    explicit TrackingZipRangeSource(
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
        ++read_calls;
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

    mutable std::size_t read_calls{0};
    mutable std::size_t max_read{0};

private:
    std::string bytes_;
};


} // namespace

int main() {
    EpubParser parser;

    ParserSourceInfo source{
        "book-epub-test",
        "/books/test.epub",
        "test.epub",
    };

    const auto metadata_only =
        parser.parseMetadata(
            sampleEpub(),
            source
        );

    assert(metadata_only.ok());
    assert(metadata_only.document.sections.empty());
    assert(
        metadata_only.document.metadata.title ==
        "ENKU & EPUB"
    );
    assert(
        metadata_only.document.metadata.authors.size() ==
        2
    );

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
        "Alex Example, Second Author"
    );
    assert(
        result.document.metadata.authors.size() ==
        2
    );
    assert(
        result.document.metadata.description ==
        std::optional<std::string>{
            "Compact reader test book."
        }
    );
    assert(
        result.document.metadata.publisher ==
        std::optional<std::string>{
            "ENKU Project"
        }
    );
    assert(
        result.document.metadata.published_date ==
        std::optional<std::string>{
            "2026-10-03"
        }
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
        result.document.sections[0].title ==
        std::optional<std::string>{
            "The First Chapter"
        }
    );
    assert(
        result.document.sections[1].title ==
        std::optional<std::string>{
            "The Second Chapter"
        }
    );
    assert(
        result.document.sections[0].blocks.size() ==
        5
    );
    assert(
        result.document.sections[0].
            blocks[0].type ==
        TextBlockType::Heading
    );
    assert(
        result.document.sections[0].
            blocks[0].text ==
        "Chapter One"
    );
    assert(
        result.document.sections[0].
            blocks[1].type ==
        TextBlockType::Paragraph
    );
    assert(
        result.document.sections[0].
            blocks[1].text ==
        "First — important paragraph."
    );
    assert(
        result.document.sections[0].
            blocks[2].type ==
        TextBlockType::Quote
    );
    assert(
        result.document.sections[0].
            blocks[3].type ==
        TextBlockType::ListItem
    );
    assert(
        result.document.sections[1].
            blocks[0].text ==
        "Chapter two text."
    );
    assert(
        result.document.total_text_length > 0
    );

    ParserSourceInfo legacy_source{
        "book-epub2-test",
        "/books/legacy.epub",
        "legacy.epub",
    };

    const auto legacy =
        parser.parse(
            sampleEpub2(),
            legacy_source
        );

    assert(legacy.ok());
    assert(
        legacy.document.metadata.title ==
        "Legacy EPUB"
    );
    assert(
        legacy.document.metadata.toc_available
    );
    assert(legacy.document.sections.size() == 1);
    assert(
        legacy.document.sections[0].title ==
        std::optional<std::string>{
            "Legacy Chapter"
        }
    );

    TrackingZipRangeSource ranged_source(
        sampleLargeRangedEpub()
    );

    assert(
        ranged_source.size() >
        512U * 1024U
    );

    const auto ranged =
        parser.parse(
            ranged_source,
            source
        );

    assert(ranged.ok());
    assert(
        ranged.document.metadata.title ==
        "Ranged EPUB"
    );
    assert(
        ranged.document.sections.size() == 1U
    );
    assert(
        ranged.document.sections[0].
            blocks.size() == 1U
    );
    assert(
        ranged.document.sections[0].
            blocks[0].type ==
        TextBlockType::Paragraph
    );
    assert(
        ranged.document.sections[0].
            blocks[0].text.size() ==
        192U * 1024U
    );
    assert(ranged_source.read_calls > 0);
    assert(
        ranged_source.max_read <= 65557U
    );

    {
        const auto bomb_bytes =
            oversizedDeclaredZip();
        ZipArchive bomb(bomb_bytes);

        assert(
            bomb.status() ==
            ZipArchiveStatus::Ok
        );

        std::string ignored;
        assert(
            bomb.read(
                "huge.xhtml",
                ignored
            ) ==
            ZipArchiveStatus::EntryTooLarge
        );
        assert(ignored.empty());
    }

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
