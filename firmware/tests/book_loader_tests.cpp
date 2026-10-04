#include "enku/reader/book_loader.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace enku;

namespace {

void appendU16(
    std::string& out,
    std::uint16_t value
) {
    out.push_back(
        static_cast<char>(
            value & 0xFFU
        )
    );
    out.push_back(
        static_cast<char>(
            (value >> 8U) & 0xFFU
        )
    );
}

void appendU32(
    std::string& out,
    std::uint32_t value
) {
    appendU16(
        out,
        static_cast<std::uint16_t>(
            value & 0xFFFFU
        )
    );
    appendU16(
        out,
        static_cast<std::uint16_t>(
            value >> 16U
        )
    );
}

std::string sampleStoredEpub() {
    struct Entry {
        std::string name;
        std::string bytes;
        std::uint32_t offset{0};
    };

    std::vector<Entry> entries{
        {
            "META-INF/container.xml",
            R"(<container><rootfiles><rootfile full-path="OEBPS/content.opf"/></rootfiles></container>)"
        },
        {
            "OEBPS/content.opf",
            R"(<package xmlns:dc="http://purl.org/dc/elements/1.1/"><metadata><dc:title>Loader EPUB</dc:title></metadata><manifest><item id="c1" href="ch1.xhtml" media-type="application/xhtml+xml"/></manifest><spine><itemref idref="c1"/></spine></package>)"
        },
        {
            "OEBPS/ch1.xhtml",
            R"(<html><body><h1>Chapter</h1><p>Range-loaded EPUB body.</p></body></html>)"
        },
    };

    std::string out;

    for (auto& entry : entries) {
        entry.offset =
            static_cast<std::uint32_t>(
                out.size()
            );

        appendU32(out, 0x04034B50U);
        appendU16(out, 20U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU32(out, 0U);
        appendU32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        appendU32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        appendU16(
            out,
            static_cast<std::uint16_t>(
                entry.name.size()
            )
        );
        appendU16(out, 0U);
        out += entry.name;
        out += entry.bytes;
    }

    const auto central_offset =
        static_cast<std::uint32_t>(
            out.size()
        );

    for (const auto& entry : entries) {
        appendU32(out, 0x02014B50U);
        appendU16(out, 20U);
        appendU16(out, 20U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU32(out, 0U);
        appendU32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        appendU32(
            out,
            static_cast<std::uint32_t>(
                entry.bytes.size()
            )
        );
        appendU16(
            out,
            static_cast<std::uint16_t>(
                entry.name.size()
            )
        );
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU16(out, 0U);
        appendU32(out, 0U);
        appendU32(out, entry.offset);
        out += entry.name;
    }

    const auto central_size =
        static_cast<std::uint32_t>(
            out.size()
        ) - central_offset;

    appendU32(out, 0x06054B50U);
    appendU16(out, 0U);
    appendU16(out, 0U);
    appendU16(
        out,
        static_cast<std::uint16_t>(
            entries.size()
        )
    );
    appendU16(
        out,
        static_cast<std::uint16_t>(
            entries.size()
        )
    );
    appendU32(out, central_size);
    appendU32(out, central_offset);
    appendU16(out, 0U);

    return out;
}

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

class FakeLibraryService final : public LibraryService {
public:
    LibraryStatus upsert(const BookRecord& value) override {
        record = value;
        return LibraryStatus::Ok;
    }

    LibraryStatus remove(const BookId& book_id) override {
        if (record.has_value() &&
            record->book_id == book_id) {
            record.reset();
            return LibraryStatus::Ok;
        }
        return LibraryStatus::NotFound;
    }

    std::optional<BookRecord> get(
        const BookId& book_id
    ) const override {
        if (!record.has_value() ||
            record->book_id != book_id) {
            return std::nullopt;
        }
        return record;
    }

    LibraryStatus query(
        const LibraryQuery&,
        LibraryPage&
    ) const override {
        return LibraryStatus::Ok;
    }

    std::optional<BookId> findByFingerprint(
        const std::string&
    ) const override {
        return std::nullopt;
    }

    LibraryStatus updateSummary(
        const BookId&,
        ReadingState,
        float,
        std::uint64_t
    ) override {
        return LibraryStatus::Ok;
    }

    std::optional<BookRecord> record;
};

class FakeBookSourceService final : public BookSourceService {
public:
    BookSourceStatus readSource(
        const BookRecord&,
        std::string& bytes
    ) override {
        ++whole_reads;
        bytes = content;
        return status;
    }

    BookSourceStatus sourceSize(
        const BookRecord&,
        std::uint64_t& size_bytes
    ) override {
        ++size_reads;

        if (status != BookSourceStatus::Ok) {
            size_bytes = 0;
            return status;
        }

        size_bytes =
            static_cast<std::uint64_t>(
                content.size()
            );
        return BookSourceStatus::Ok;
    }

    BookSourceStatus readSourceRange(
        const BookRecord&,
        std::uint64_t offset,
        std::size_t length,
        std::string& bytes
    ) override {
        ++range_reads;

        if (status != BookSourceStatus::Ok) {
            bytes.clear();
            return status;
        }

        if (offset >
                static_cast<std::uint64_t>(
                    content.size()
                ) ||
            static_cast<std::uint64_t>(
                length
            ) >
                static_cast<std::uint64_t>(
                    content.size()
                ) -
                    offset) {
            bytes.clear();
            return BookSourceStatus::ReadFailed;
        }

        bytes.assign(
            content,
            static_cast<std::size_t>(
                offset
            ),
            length
        );

        return BookSourceStatus::Ok;
    }

    BookSourceStatus validateSource(
        const BookRecord&,
        bool& matches
    ) override {
        ++validation_reads;

        if (status != BookSourceStatus::Ok) {
            matches = false;
            return status;
        }

        matches = source_matches;
        return BookSourceStatus::Ok;
    }

    BookSourceStatus status{BookSourceStatus::Ok};
    bool source_matches{true};
    std::string content;
    std::uint32_t validation_reads{0};
    std::uint32_t whole_reads{0};
    std::uint32_t size_reads{0};
    std::uint32_t range_reads{0};
};

} // namespace

int main() {
    FixedWidthMeasurer measurer;
    FakeLibraryService library;
    FakeBookSourceService source;

    BookRecord record;
    record.book_id = "loader-test";
    record.format = BookFormat::Txt;
    record.source_path = "/books/loader.txt";
    record.source_filename = "loader.txt";
    record.metadata.title = "loader";
    library.record = record;

    source.content =
        "Alpha beta gamma delta epsilon zeta eta theta iota kappa.";

    ReaderBookLoader loader(library, source, measurer);

    BookLoadRequest request{
        "loader-test",
        std::nullopt,
        TypographySettings{16, 1.0F, 10},
        Viewport{140, 80},
    };

    source.validation_reads = 0;
    source.whole_reads = 0;
    source.size_reads = 0;
    source.range_reads = 0;

    const auto opened = loader.open(request);
    assert(opened.ok());
    assert(source.validation_reads == 1);
    assert(source.whole_reads == 0);
    assert(source.size_reads == 1);
    assert(source.range_reads > 0);
    assert(loader.session() != nullptr);
    assert(loader.session()->isOpen());
    assert(loader.document() != nullptr);
    assert(
        loader.document()->book_id ==
        "loader-test"
    );

    const auto first_position =
        loader.session()->currentPage()->first_position;

    loader.close();
    assert(loader.session() == nullptr);
    assert(loader.document() == nullptr);

    request.saved_position = first_position;
    const auto reopened = loader.open(request);
    assert(reopened.ok());
    assert(loader.session() != nullptr);

    request.saved_position =
        SemanticPosition{
            "loader-test",
            "missing-section",
            999999U,
        };

    const auto stale_section =
        loader.open(request);

    assert(stale_section.ok());
    assert(loader.session() != nullptr);
    assert(
        loader.session()->currentPage().has_value()
    );
    assert(
        loader.session()->currentPage()->
            first_position.section_id ==
        first_position.section_id
    );
    assert(
        loader.session()->currentPage()->
            first_position.text_offset ==
        first_position.text_offset
    );

    request.saved_position =
        SemanticPosition{
            "loader-test",
            first_position.section_id,
            999999U,
        };

    const auto stale_offset =
        loader.open(request);

    assert(stale_offset.ok());
    assert(loader.session() != nullptr);
    assert(
        loader.session()->currentPage().has_value()
    );
    assert(
        loader.session()->currentPage()->
            first_position.text_offset ==
        first_position.text_offset
    );

    source.status = BookSourceStatus::Unavailable;
    const auto unavailable = loader.open(request);
    assert(
        unavailable.status ==
        BookLoadStatus::SourceUnavailable
    );
    assert(loader.session() == nullptr);

    source.status = BookSourceStatus::Ok;
    source.source_matches = false;
    const auto changed = loader.open(request);
    assert(
        changed.status ==
        BookLoadStatus::SourceChanged
    );
    assert(loader.session() == nullptr);
    assert(loader.document() == nullptr);

    source.source_matches = true;
    request.saved_position.reset();
    library.record->format = BookFormat::Epub;
    source.content = sampleStoredEpub();
    source.whole_reads = 0;
    source.size_reads = 0;
    source.range_reads = 0;

    const auto epub_opened =
        loader.open(request);

    assert(epub_opened.ok());
    assert(source.whole_reads == 0);
    assert(source.size_reads == 1);
    assert(source.range_reads > 0);
    assert(loader.document() != nullptr);
    assert(
        loader.document()->metadata.title ==
        "Loader EPUB"
    );

    library.record->format =
        BookFormat::Fb2;
    library.record->source_path =
        "/books/loader.fb2";
    library.record->source_filename =
        "loader.fb2";

    source.content =
        R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info>
   <book-title>Loader FB2</book-title>
  </title-info>
 </description>
 <body>
  <section>
   <title><p>FB2 Chapter</p></title>
   <p>Range-loaded FB2 body.</p>
  </section>
 </body>
</FictionBook>)";

    source.whole_reads = 0;
    source.size_reads = 0;
    source.range_reads = 0;

    const auto fb2_opened =
        loader.open(request);

    assert(fb2_opened.ok());
    assert(source.whole_reads == 0);
    assert(source.size_reads == 1);
    assert(source.range_reads > 0);
    assert(loader.document() != nullptr);
    assert(
        loader.document()->metadata.title ==
        "Loader FB2"
    );
    assert(
        loader.document()->sections.size() ==
        1U
    );
    assert(
        loader.document()->sections[0].title ==
        std::optional<std::string>{
            "FB2 Chapter"
        }
    );

    library.record->format =
        BookFormat::Epub;
    library.record->source_path =
        "/books/loader.epub";
    library.record->source_filename =
        "loader.epub";

    source.content = "not an epub";
    const auto invalid_epub =
        loader.open(request);
    assert(
        invalid_epub.status ==
        BookLoadStatus::ParseFailed
    );

    library.record.reset();
    const auto missing = loader.open(request);
    assert(missing.status == BookLoadStatus::NotFound);

    return 0;
}
