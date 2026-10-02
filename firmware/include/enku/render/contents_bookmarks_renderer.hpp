#pragma once

#include <string>
#include <vector>

#include "../core/app_state.hpp"
#include "../core/bookmarks.hpp"

namespace enku {

struct ContentsEntry {
    SemanticPosition position;
    std::string label;
};

class ContentsBookmarksRenderer {
public:
    virtual ~ContentsBookmarksRenderer() = default;

    virtual bool renderContentsBookmarks(
        const AppState& app_state,
        const std::vector<ContentsEntry>& contents,
        const std::vector<BookmarkRecord>& bookmarks
    ) = 0;
};

} // namespace enku
