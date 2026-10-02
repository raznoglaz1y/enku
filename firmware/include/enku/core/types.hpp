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
    Boot = 0,
    Library = 1,
    BookOpening = 2,
    Reading = 3,
    ReaderOverlay = 4,
    Search = 5,
    Settings = 6,
    ImportTransfer = 7,
    Sleep = 8,
    ErrorRecovery = 9,

    // Transient UI screens are appended so persisted legacy values remain
    // stable across firmware upgrades.
    BookDetails = 10,
    BookFinished = 11,
    ContentsBookmarks = 12,
    AboutBook = 13,
    ReadingSettings = 14,
    DisplaySettings = 15,
    LocaleSettings = 16,
    AboutDevice = 17,
    PowerOffConfirm = 18,
};

static_assert(
    static_cast<std::uint8_t>(Screen::Library) == 1,
    "Persisted Screen::Library id must remain stable"
);
static_assert(
    static_cast<std::uint8_t>(Screen::Reading) == 3,
    "Persisted Screen::Reading id must remain stable"
);
static_assert(
    static_cast<std::uint8_t>(Screen::ErrorRecovery) == 9,
    "Legacy persisted Screen range must remain stable"
);

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
