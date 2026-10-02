#pragma once

#include <vector>

#include "../core/bookmarks.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborBookmarkService {
public:
    explicit CborBookmarkService(
        StateFileStore& files
    );

    BookmarkStatus load(
        const BookId& book_id,
        std::vector<BookmarkRecord>& bookmarks
    ) const;

    BookmarkStatus add(
        const BookmarkRecord& bookmark
    );

    BookmarkStatus remove(
        const SemanticPosition& position
    );

    BookmarkStatus replace(
        const BookId& book_id,
        const std::vector<BookmarkRecord>& bookmarks
    );

    BookmarkStatus eraseBook(
        const BookId& book_id
    );

private:
    StateFileStore& files_;

    static std::string pathFor(
        const BookId& book_id
    );

    static bool samePosition(
        const SemanticPosition& a,
        const SemanticPosition& b
    );
};

} // namespace enku
