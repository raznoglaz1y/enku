#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace enku {

enum class BookFormat : std::uint8_t {
    Epub,
    Fb2,
    Txt,
};

struct BookMetadata {
    std::string title;
    std::string author_display;
    std::vector<std::string> authors;
    std::optional<std::string> language;
    std::optional<std::string> description;
    std::optional<std::string> series_name;
    std::optional<float> series_index;
    std::optional<std::string> publisher;
    std::optional<std::string> published_date;
    std::optional<std::string> identifier;
    bool toc_available{false};
};

struct BookRecord {
    BookId book_id;
    BookFormat format{BookFormat::Epub};
    BookMetadata metadata;

    std::string source_path;
    std::string source_filename;
    std::uint64_t file_size{0};
    std::string fingerprint;

    ReadingState reading_state{ReadingState::New};
    float progress{0.0F};
    std::uint64_t added_order{0};
    std::uint64_t last_opened_order{0};
    std::optional<std::string> cover_cache_ref;
};

enum class LibraryFilter : std::uint8_t {
    All,
    New,
    Reading,
    Finished,
};

enum class LibrarySort : std::uint8_t {
    Title,
    Author,
    RecentlyOpened,
    RecentlyAdded,
};

enum class SortDirection : std::uint8_t {
    Ascending,
    Descending,
};

enum class LibraryQueryMode : std::uint8_t {
    Browse,
    Search,
};

struct LibraryQuery {
    LibraryQueryMode mode{LibraryQueryMode::Browse};
    LibraryFilter filter{LibraryFilter::All};
    LibrarySort sort{LibrarySort::RecentlyOpened};
    SortDirection direction{SortDirection::Descending};
    std::string search_text;
    std::uint32_t offset{0};
    std::uint16_t limit{24};
};

struct LibraryPage {
    std::vector<BookRecord> items;
    std::uint32_t total_matches{0};
    std::uint32_t offset{0};
};

enum class LibraryStatus : std::uint8_t {
    Ok,
    NotFound,
    StorageUnavailable,
    InvalidRecord,
    PersistenceFailure,
    NoSpace,
    Busy,
    InvalidQuery,
};

} // namespace enku
