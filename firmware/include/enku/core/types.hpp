#pragma once

#include <cstdint>
#include <string>

namespace enku {

using BookId = std::string;
using BookmarkId = std::string;

enum class Orientation : std::uint8_t {
    Portrait,
    Landscape,
};

enum class Screen : std::uint8_t {
    Boot,
    Library,
    BookDetails,
    BookOpening,
    Reading,
    BookFinished,
    ReaderOverlay,
    Search,
    Settings,
    ImportTransfer,
    Sleep,
    ErrorRecovery,
};

enum class ReadingState : std::uint8_t {
    New,
    Reading,
    Finished,
};

enum class LibraryView : std::uint8_t {
    Grid,
    List,
};

struct SemanticPosition {
    BookId book_id;
    std::string section_id;
    std::uint64_t text_offset{0};
};

struct Viewport {
    std::uint16_t width{0};
    std::uint16_t height{0};
};

} // namespace enku
