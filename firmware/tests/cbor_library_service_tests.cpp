#include "enku/storage/cbor_library_service.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace enku;

namespace {

class MemoryStateFileStore final : public StateFileStore {
public:
    StateFileStatus read(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    ) override {
        const auto it = files.find(path);
        if (it == files.end()) {
            return StateFileStatus::NotFound;
        }

        bytes = it->second;
        return StateFileStatus::Ok;
    }

    StateFileStatus write(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    ) override {
        if (write_status != StateFileStatus::Ok) {
            return write_status;
        }

        files[path] = bytes;
        last_write_path = path;
        ++writes;
        return StateFileStatus::Ok;
    }

    std::map<std::string, std::vector<std::uint8_t>> files;
    StateFileStatus write_status{StateFileStatus::Ok};
    std::string last_write_path;
    std::uint32_t writes{0};
};

BookRecord makeBook(
    std::string id,
    std::string title,
    std::string author,
    std::string fingerprint,
    ReadingState state,
    float progress,
    std::uint64_t added,
    std::uint64_t opened
) {
    BookRecord record;
    record.book_id = std::move(id);
    record.format = BookFormat::Txt;
    record.metadata.title = std::move(title);
    record.metadata.author_display = author;
    record.metadata.authors = {std::move(author)};
    record.metadata.language = "en";
    record.metadata.description = "description";
    record.metadata.series_name = "series";
    record.metadata.series_index = 1.0F;
    record.metadata.publisher = "publisher";
    record.metadata.published_date = "2026";
    record.metadata.identifier = "identifier";
    record.metadata.toc_available = false;
    record.source_path = "/books/" + record.book_id + ".txt";
    record.source_filename = record.book_id + ".txt";
    record.file_size = 100;
    record.fingerprint = std::move(fingerprint);
    record.reading_state = state;
    record.progress = progress;
    record.added_order = added;
    record.last_opened_order = opened;
    record.cover_cache_ref =
        "/system/covers/" + record.book_id;
    return record;
}

} // namespace

int main() {
    MemoryStateFileStore files;
    CborLibraryService library(files);

    assert(library.load() == LibraryStatus::Ok);
    assert(library.records().empty());

    const auto alpha = makeBook(
        "alpha",
        "Alpha",
        "Ada",
        "fp-alpha",
        ReadingState::New,
        0.0F,
        1,
        0
    );

    const auto beta = makeBook(
        "beta",
        "Beta",
        "Boris",
        "fp-beta",
        ReadingState::Reading,
        0.5F,
        2,
        20
    );

    const auto gamma = makeBook(
        "gamma",
        "Gamma",
        "Ada",
        "fp-gamma",
        ReadingState::Finished,
        1.0F,
        3,
        10
    );

    assert(library.upsert(alpha) == LibraryStatus::Ok);
    assert(library.upsert(beta) == LibraryStatus::Ok);
    assert(library.upsert(gamma) == LibraryStatus::Ok);
    assert(files.writes == 3);

    const auto found = library.get("beta");
    assert(found.has_value());
    assert(found->metadata.title == "Beta");
    assert(found->metadata.series_index.has_value());
    assert(*found->metadata.series_index == 1.0F);

    assert(
        library.findByFingerprint("fp-gamma") ==
        std::optional<BookId>{"gamma"}
    );

    LibraryPage page;

    LibraryQuery reading_query;
    reading_query.mode = LibraryQueryMode::Browse;
    reading_query.filter = LibraryFilter::Reading;
    reading_query.sort = LibrarySort::Title;
    reading_query.direction = SortDirection::Ascending;

    assert(
        library.query(reading_query, page) ==
        LibraryStatus::Ok
    );
    assert(page.total_matches == 1);
    assert(page.items.size() == 1);
    assert(page.items[0].book_id == "beta");

    // Search is global and ignores the browse reading-state filter.
    LibraryQuery search_query;
    search_query.mode = LibraryQueryMode::Search;
    search_query.filter = LibraryFilter::Reading;
    search_query.search_text = "ada";
    search_query.sort = LibrarySort::RecentlyAdded;
    search_query.direction = SortDirection::Ascending;

    assert(
        library.query(search_query, page) ==
        LibraryStatus::Ok
    );
    assert(page.total_matches == 2);
    assert(page.items.size() == 2);
    assert(page.items[0].book_id == "alpha");
    assert(page.items[1].book_id == "gamma");

    LibraryQuery paged;
    paged.mode = LibraryQueryMode::Browse;
    paged.filter = LibraryFilter::All;
    paged.sort = LibrarySort::RecentlyAdded;
    paged.direction = SortDirection::Ascending;
    paged.offset = 1;
    paged.limit = 1;

    assert(
        library.query(paged, page) ==
        LibraryStatus::Ok
    );
    assert(page.total_matches == 3);
    assert(page.items.size() == 1);
    assert(page.items[0].book_id == "beta");

    assert(
        library.updateSummary(
            "alpha",
            ReadingState::Reading,
            0.25F,
            30
        ) == LibraryStatus::Ok
    );

    assert(library.get("alpha")->progress == 0.25F);
    assert(
        library.get("alpha")->reading_state ==
        ReadingState::Reading
    );

    // Recreate service: state must come back from persisted A/B slots.
    CborLibraryService reopened(files);
    assert(reopened.load() == LibraryStatus::Ok);
    assert(reopened.records().size() == 3);
    assert(reopened.get("alpha")->progress == 0.25F);

    // Corrupt newest slot and verify fallback to the previous valid index.
    const auto newest_path = files.last_write_path;
    assert(!files.files[newest_path].empty());
    files.files[newest_path].back() ^= 0xFFU;

    CborLibraryService recovered(files);
    assert(recovered.load() == LibraryStatus::Ok);
    assert(recovered.records().size() == 3);

    // A failed commit must roll back the in-memory mutation.
    files.write_status = StateFileStatus::NoSpace;
    const auto before = recovered.get("beta")->progress;

    assert(
        recovered.updateSummary(
            "beta",
            ReadingState::Finished,
            1.0F,
            99
        ) == LibraryStatus::NoSpace
    );
    assert(recovered.get("beta")->progress == before);

    return 0;
}
