#pragma once

#include <cstdint>
#include <vector>

#include "../core/app_state.hpp"
#include "../core/bookmarks.hpp"
#include "../render/contents_bookmarks_renderer.hpp"
#include "../storage/cbor_bookmark_service.hpp"
#include "application_reader_runtime.hpp"

namespace enku {

enum class ContentsBookmarksRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class ContentsBookmarksRuntime {
public:
    ContentsBookmarksRuntime(
        AppState& app_state,
        ApplicationReaderRuntime& reader,
        CborBookmarkService& bookmarks,
        ContentsBookmarksRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    ContentsBookmarksRuntimeResult openFromReader();

    ContentsBookmarksRuntimeResult handle(
        LogicalAction action
    );

    ContentsBookmarksRuntimeResult addCurrentBookmark();

    const std::vector<ContentsEntry>& contents() const;
    const std::vector<BookmarkRecord>& bookmarks() const;

private:
    AppState& app_state_;
    ApplicationReaderRuntime& reader_;
    CborBookmarkService& bookmark_store_;
    ContentsBookmarksRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};

    std::vector<ContentsEntry> contents_;
    std::vector<BookmarkRecord> bookmarks_;
    std::uint32_t refresh_generation_{0};

    ContentsBookmarksRuntimeResult reload();
    ContentsBookmarksRuntimeResult render();
    ContentsBookmarksRuntimeResult navigate(int direction);
    ContentsBookmarksRuntimeResult confirm();
    ContentsBookmarksRuntimeResult close();
    ContentsBookmarksRuntimeResult openPosition(
        const SemanticPosition& position
    );

    std::size_t activeItemCount() const;
    void normalizeWindow();
};

} // namespace enku
