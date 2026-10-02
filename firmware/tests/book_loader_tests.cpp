#include "enku/reader/book_loader.hpp"

#include <cassert>
#include <optional>
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

class FakeLibraryService final : public LibraryService {
public:
    LibraryStatus upsert(const BookRecord& value) override {
        record = value;
        return LibraryStatus::Ok;
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
        ++calls;
        bytes = content;
        return status;
    }

    BookSourceStatus status{BookSourceStatus::Ok};
    std::string content;
    std::uint32_t calls{0};
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

    const auto opened = loader.open(request);
    assert(opened.ok());
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

    source.status = BookSourceStatus::Unavailable;
    const auto unavailable = loader.open(request);
    assert(
        unavailable.status ==
        BookLoadStatus::SourceUnavailable
    );
    assert(loader.session() == nullptr);

    source.status = BookSourceStatus::Ok;
    library.record->format = BookFormat::Epub;
    const auto unsupported = loader.open(request);
    assert(
        unsupported.status ==
        BookLoadStatus::UnsupportedFormat
    );

    library.record.reset();
    const auto missing = loader.open(request);
    assert(missing.status == BookLoadStatus::NotFound);

    return 0;
}
