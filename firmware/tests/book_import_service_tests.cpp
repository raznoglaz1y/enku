#include "enku/storage/book_import_service.hpp"

#include <cassert>
#include <optional>
#include <string>
#include <vector>

using namespace enku;

namespace {

class FakeLibraryService final : public LibraryService {
public:
    LibraryStatus upsert(
        const BookRecord& value
    ) override {
        ++upserts;

        if (upsert_status != LibraryStatus::Ok) {
            return upsert_status;
        }

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
        const std::string& fingerprint
    ) const override {
        if (record.has_value() &&
            record->fingerprint == fingerprint) {
            return record->book_id;
        }
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

    LibraryStatus upsert_status{LibraryStatus::Ok};
    std::optional<BookRecord> record;
    std::uint32_t upserts{0};
};

} // namespace

int main() {
    FakeLibraryService library;
    BookImportService importer(library);

    const BookImportSource source{
        "/incoming/The Example.txt",
        "The Example.txt",
        "First paragraph.\n\nSecond paragraph."
    };

    const auto imported = importer.import(source, 42);

    assert(imported.ok());
    assert(!imported.book_id.empty());
    assert(!imported.fingerprint.empty());
    assert(library.upserts == 1);
    assert(library.record.has_value());
    assert(library.record->book_id == imported.book_id);
    assert(library.record->format == BookFormat::Txt);
    assert(
        library.record->metadata.title ==
        "The Example"
    );
    assert(
        library.record->metadata.author_display ==
        "Unknown author"
    );
    assert(library.record->file_size == source.bytes.size());
    assert(
        library.record->fingerprint ==
        imported.fingerprint
    );
    assert(
        library.record->reading_state ==
        ReadingState::New
    );
    assert(library.record->progress == 0.0F);
    assert(library.record->added_order == 42);

    // Exact content duplicate is rejected before another Library commit.
    const auto duplicate = importer.import(source, 43);
    assert(
        duplicate.status ==
        BookImportStatus::Duplicate
    );
    assert(library.upserts == 1);

    // Same filename with different bytes is a different content identity.
    const BookImportSource changed{
        source.source_path,
        source.source_filename,
        "Different contents."
    };

    const auto changed_result =
        importer.import(changed, 44);
    assert(changed_result.ok());
    assert(
        changed_result.fingerprint !=
        imported.fingerprint
    );
    assert(
        changed_result.book_id !=
        imported.book_id
    );

    const BookImportSource invalid_utf8{
        "/incoming/bad.txt",
        "bad.txt",
        std::string("\xC3\x28", 2)
    };

    assert(
        importer.import(invalid_utf8, 45).status ==
        BookImportStatus::ParseFailed
    );

    const BookImportSource valid_fb2{
        "/incoming/reader.fb2",
        "reader.fb2",
        R"(<?xml version="1.0" encoding="utf-8"?>
<FictionBook>
 <description>
  <title-info>
   <book-title>Imported FB2</book-title>
   <author>
    <first-name>Test</first-name>
    <last-name>Author</last-name>
   </author>
  </title-info>
 </description>
 <body>
  <section>
   <title><p>Chapter</p></title>
   <p>Imported body.</p>
  </section>
 </body>
</FictionBook>)"
    };

    const auto fb2_import =
        importer.import(valid_fb2, 46);

    assert(fb2_import.ok());
    assert(library.record.has_value());
    assert(
        library.record->format ==
        BookFormat::Fb2
    );
    assert(
        library.record->metadata.title ==
        "Imported FB2"
    );

    const BookImportSource invalid_epub{
        "/incoming/book.epub",
        "book.epub",
        "epub bytes"
    };

    assert(
        importer.import(invalid_epub, 47).status ==
        BookImportStatus::ParseFailed
    );

    const BookImportSource empty{
        "/incoming/empty.txt",
        "empty.txt",
        ""
    };

    assert(
        importer.import(empty, 48).status ==
        BookImportStatus::EmptySource
    );

    library.upsert_status =
        LibraryStatus::PersistenceFailure;

    const BookImportSource commit_fail{
        "/incoming/fail.txt",
        "fail.txt",
        "Valid but cannot commit."
    };

    assert(
        importer.import(commit_fail, 49).status ==
        BookImportStatus::LibraryCommitFailed
    );

    return 0;
}
