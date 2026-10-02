#include "enku/runtime/contents_bookmarks_runtime.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

#include "enku/core/events.hpp"
#include "enku/reader/document.hpp"

namespace enku {

ContentsBookmarksRuntime::ContentsBookmarksRuntime(
    AppState& app_state,
    ApplicationReaderRuntime& reader,
    CborBookmarkService& bookmarks,
    ContentsBookmarksRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      reader_(reader),
      bookmark_store_(bookmarks),
      renderer_(renderer),
      refresh_(refresh) {}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::openFromReader() {
    if (app_state_.screen != Screen::Reading ||
        !app_state_.current_book.has_value() ||
        reader_.loader().document() == nullptr) {
        return ContentsBookmarksRuntimeResult::Ignored;
    }

    app_state_.contents_bookmarks =
        ContentsBookmarksState{};

    app_state_.screen =
        Screen::ContentsBookmarks;

    return reload();
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::reload() {
    if (!app_state_.current_book.has_value()) {
        return ContentsBookmarksRuntimeResult::Failed;
    }

    const auto* document =
        reader_.loader().document();

    if (document == nullptr ||
        document->book_id !=
            *app_state_.current_book) {
        return ContentsBookmarksRuntimeResult::Failed;
    }

    contents_.clear();

    for (const auto& section : document->sections) {
        if (!section.title.has_value() ||
            section.title->empty()) {
            continue;
        }

        std::uint64_t offset = 0;

        if (!section.blocks.empty()) {
            offset =
                section.blocks.front().text_offset;
        }

        contents_.push_back(
            ContentsEntry{
                SemanticPosition{
                    document->book_id,
                    section.id,
                    offset,
                },
                *section.title,
            }
        );
    }

    bookmarks_.clear();

    const auto bookmark_status =
        bookmark_store_.load(
            document->book_id,
            bookmarks_
        );

    if (bookmark_status != BookmarkStatus::Ok &&
        bookmark_status != BookmarkStatus::NotFound) {
        return ContentsBookmarksRuntimeResult::Failed;
    }

    app_state_.contents_bookmarks.item_index = 0;
    app_state_.contents_bookmarks.window_start = 0;

    return render();
}

std::size_t
ContentsBookmarksRuntime::activeItemCount() const {
    return app_state_.contents_bookmarks.tab ==
            ContentsBookmarksTab::Contents
        ? contents_.size()
        : bookmarks_.size();
}

void ContentsBookmarksRuntime::normalizeWindow() {
    const auto count = activeItemCount();

    if (count == 0U) {
        app_state_.contents_bookmarks.item_index = 0;
        app_state_.contents_bookmarks.window_start = 0;
        return;
    }

    if (app_state_.contents_bookmarks.item_index >=
        count) {
        app_state_.contents_bookmarks.item_index =
            static_cast<std::uint32_t>(
                count - 1U
            );
    }

    const std::uint32_t visible =
        app_state_.orientation ==
                Orientation::Portrait
            ? 8U
            : 5U;

    auto& window =
        app_state_.contents_bookmarks.window_start;

    const auto item =
        app_state_.contents_bookmarks.item_index;

    if (item < window) {
        window = item;
    } else if (item >= window + visible) {
        window =
            item - visible + 1U;
    }
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::navigate(
    int direction
) {
    auto& state =
        app_state_.contents_bookmarks;

    std::uint32_t linear = 0;

    switch (state.focus) {
        case ContentsBookmarksFocus::ContentsTab:
            linear = 0;
            break;

        case ContentsBookmarksFocus::BookmarksTab:
            linear = 1;
            break;

        case ContentsBookmarksFocus::Item:
            linear = 2U + state.item_index;
            break;
    }

    const auto item_count =
        static_cast<std::uint32_t>(
            activeItemCount()
        );

    const std::uint32_t total =
        2U + item_count;

    int next =
        static_cast<int>(linear) +
        direction;

    if (next < 0) {
        next =
            static_cast<int>(total) - 1;
    } else if (
        next >= static_cast<int>(total)
    ) {
        next = 0;
    }

    if (next == 0) {
        state.focus =
            ContentsBookmarksFocus::ContentsTab;
    } else if (next == 1) {
        state.focus =
            ContentsBookmarksFocus::BookmarksTab;
    } else {
        state.focus =
            ContentsBookmarksFocus::Item;
        state.item_index =
            static_cast<std::uint32_t>(
                next - 2
            );
        normalizeWindow();
    }

    return render();
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::confirm() {
    auto& state =
        app_state_.contents_bookmarks;

    if (state.focus ==
        ContentsBookmarksFocus::ContentsTab) {
        state.tab =
            ContentsBookmarksTab::Contents;
        state.item_index = 0;
        state.window_start = 0;
        return render();
    }

    if (state.focus ==
        ContentsBookmarksFocus::BookmarksTab) {
        state.tab =
            ContentsBookmarksTab::Bookmarks;
        state.item_index = 0;
        state.window_start = 0;
        return render();
    }

    const auto index =
        static_cast<std::size_t>(
            state.item_index
        );

    if (state.tab ==
        ContentsBookmarksTab::Contents) {
        if (index >= contents_.size()) {
            return ContentsBookmarksRuntimeResult::Ignored;
        }

        return openPosition(
            contents_[index].position
        );
    }

    if (index >= bookmarks_.size()) {
        return ContentsBookmarksRuntimeResult::Ignored;
    }

    return openPosition(
        bookmarks_[index].position
    );
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::openPosition(
    const SemanticPosition& position
) {
    app_state_.screen = Screen::Reading;

    const auto result =
        reader_.reader().handle(
            BookPositionChanged{
                position
            }
        );

    if (result != ReaderRuntimeResult::Applied) {
        app_state_.screen =
            Screen::ContentsBookmarks;
        return ContentsBookmarksRuntimeResult::Failed;
    }

    return ContentsBookmarksRuntimeResult::Applied;
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::close() {
    app_state_.screen = Screen::Reading;

    const auto redraw =
        reader_.reader().redrawCurrentPage(
            RefreshReason::OverlayChanged
        );

    if (redraw != ReaderRuntimeResult::Applied) {
        app_state_.screen =
            Screen::ContentsBookmarks;
        return ContentsBookmarksRuntimeResult::Failed;
    }

    return ContentsBookmarksRuntimeResult::Applied;
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::addCurrentBookmark() {
    if (!app_state_.current_book.has_value() ||
        !app_state_.reading_position.has_value()) {
        return ContentsBookmarksRuntimeResult::Ignored;
    }

    std::string label = "Bookmark";

    const auto* document =
        reader_.loader().document();

    if (document != nullptr) {
        const auto section =
            std::find_if(
                document->sections.begin(),
                document->sections.end(),
                [&](const DocumentSection& value) {
                    return value.id ==
                        app_state_.reading_position->
                            section_id;
                }
            );

        if (section != document->sections.end() &&
            section->title.has_value() &&
            !section->title->empty()) {
            label = *section->title;
        } else if (
            !app_state_.reading_position->
                section_id.empty()) {
            label =
                app_state_.reading_position->
                    section_id;
        }
    }

    const auto status =
        bookmark_store_.add(
            BookmarkRecord{
                *app_state_.reading_position,
                label,
            }
        );

    return status == BookmarkStatus::Ok ||
           status == BookmarkStatus::AlreadyExists
        ? ContentsBookmarksRuntimeResult::Applied
        : ContentsBookmarksRuntimeResult::Failed;
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderContentsBookmarks(
            app_state_,
            contents_,
            bookmarks_
        )) {
        return ContentsBookmarksRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return ContentsBookmarksRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::OverlayChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? ContentsBookmarksRuntimeResult::Applied
        : ContentsBookmarksRuntimeResult::Failed;
}

ContentsBookmarksRuntimeResult
ContentsBookmarksRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen !=
        Screen::ContentsBookmarks) {
        return ContentsBookmarksRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return navigate(-1);

        case LogicalAction::NavigateNext:
            return navigate(1);

        case LogicalAction::Confirm:
            return confirm();

        case LogicalAction::Back:
            return close();

        default:
            return ContentsBookmarksRuntimeResult::Ignored;
    }
}

const std::vector<ContentsEntry>&
ContentsBookmarksRuntime::contents() const {
    return contents_;
}

const std::vector<BookmarkRecord>&
ContentsBookmarksRuntime::bookmarks() const {
    return bookmarks_;
}

} // namespace enku
