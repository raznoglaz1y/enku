#include "enku/storage/cbor_bookmark_service.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

using namespace enku;

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-bookmark-service-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore files(root);
    CborBookmarkService bookmarks(files);

    const BookmarkRecord first{
        SemanticPosition{
            "book-a",
            "chapter-1",
            120,
        },
        "Chapter 1",
    };

    const BookmarkRecord second{
        SemanticPosition{
            "book-a",
            "chapter-2",
            40,
        },
        "Chapter 2",
    };

    std::vector<BookmarkRecord> loaded;

    assert(
        bookmarks.load(
            "book-a",
            loaded
        ) == BookmarkStatus::NotFound
    );
    assert(loaded.empty());

    assert(
        bookmarks.add(first) ==
        BookmarkStatus::Ok
    );
    assert(
        bookmarks.add(second) ==
        BookmarkStatus::Ok
    );
    assert(
        bookmarks.add(first) ==
        BookmarkStatus::AlreadyExists
    );

    assert(
        bookmarks.load(
            "book-a",
            loaded
        ) == BookmarkStatus::Ok
    );
    assert(loaded.size() == 2);
    assert(
        loaded[0].position.section_id ==
        "chapter-1"
    );
    assert(
        loaded[0].position.text_offset ==
        120
    );
    assert(loaded[0].label == "Chapter 1");
    assert(
        loaded[1].position.section_id ==
        "chapter-2"
    );

    assert(
        bookmarks.remove(first.position) ==
        BookmarkStatus::Ok
    );

    assert(
        bookmarks.load(
            "book-a",
            loaded
        ) == BookmarkStatus::Ok
    );
    assert(loaded.size() == 1);
    assert(
        loaded[0].position.section_id ==
        "chapter-2"
    );

    assert(
        bookmarks.remove(first.position) ==
        BookmarkStatus::NotFound
    );

    assert(
        bookmarks.eraseBook("book-a") ==
        BookmarkStatus::Ok
    );
    assert(
        bookmarks.load(
            "book-a",
            loaded
        ) == BookmarkStatus::NotFound
    );

    // Cross-book replacement is rejected.
    const BookmarkRecord foreign{
        SemanticPosition{
            "book-b",
            "chapter-9",
            1,
        },
        "Foreign",
    };

    assert(
        bookmarks.replace(
            "book-a",
            std::vector<BookmarkRecord>{foreign}
        ) == BookmarkStatus::Invalid
    );

    std::filesystem::remove_all(root, ec);
    return 0;
}
