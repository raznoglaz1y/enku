#include "enku/runtime/library_runtime.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace enku {

LibraryRuntimeController::LibraryRuntimeController(
    AppState& app_state,
    LibraryService& library,
    ReaderRuntimeController& reader,
    StagedBookImportService& importer,
    BookDeleteService& deleter,
    RefreshService& refresh
)
    : app_state_(app_state),
      library_(library),
      reader_(reader),
      importer_(importer),
      deleter_(deleter),
      refresh_(refresh) {}

const LibraryPage& LibraryRuntimeController::page() const {
    return page_;
}

StagedImportStatus
LibraryRuntimeController::lastImportStatus() const {
    return last_import_status_;
}

std::uint32_t
LibraryRuntimeController::refreshGeneration() const {
    return refresh_generation_;
}

LibraryQuery LibraryRuntimeController::queryFromState() const {
    LibraryQuery query;
    query.mode = app_state_.library.mode;
    query.filter = app_state_.library.filter;
    query.sort = app_state_.library.sort;
    query.direction = app_state_.library.direction;
    query.search_text = app_state_.library.search_text;
    query.offset = app_state_.library.offset;
    query.limit = app_state_.library.limit;
    return query;
}

LibraryRuntimeResult
LibraryRuntimeController::submitRefresh(
    RefreshReason reason,
    RefreshClass refresh_class
) {
    RefreshRequest request;
    request.refresh_class = refresh_class;
    request.reason = reason;
    request.generation = ++refresh_generation_;
    request.may_coalesce = true;
    request.may_defer = false;

    if (!refresh_.submit(request)) {
        return LibraryRuntimeResult::RefreshRejected;
    }

    return LibraryRuntimeResult::Applied;
}

void LibraryRuntimeController::normalizeFocus() {
    app_state_.library.total_matches =
        page_.total_matches;

    if (page_.items.empty()) {
        app_state_.library.focused_book.reset();
        return;
    }

    if (app_state_.library.focused_book.has_value()) {
        const auto found = std::find_if(
            page_.items.begin(),
            page_.items.end(),
            [&](const BookRecord& record) {
                return record.book_id ==
                    *app_state_.library.focused_book;
            }
        );

        if (found != page_.items.end()) {
            return;
        }
    }

    app_state_.library.focused_book =
        page_.items.front().book_id;
}

std::optional<std::size_t>
LibraryRuntimeController::focusedIndex() const {
    if (!app_state_.library.focused_book.has_value()) {
        return std::nullopt;
    }

    const auto it = std::find_if(
        page_.items.begin(),
        page_.items.end(),
        [&](const BookRecord& record) {
            return record.book_id ==
                *app_state_.library.focused_book;
        }
    );

    if (it == page_.items.end()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(
        std::distance(page_.items.begin(), it)
    );
}

LibraryRuntimeResult LibraryRuntimeController::reload(
    RefreshReason reason
) {
    LibraryPage next_page;
    const auto status =
        library_.query(queryFromState(), next_page);

    if (status != LibraryStatus::Ok) {
        return LibraryRuntimeResult::QueryFailed;
    }

    page_ = std::move(next_page);
    normalizeFocus();

    if (app_state_.screen != Screen::Library) {
        app_state_.screen = Screen::Library;
    }

    const auto refreshed =
        submitRefresh(reason, RefreshClass::Full);

    if (refreshed != LibraryRuntimeResult::Applied) {
        return refreshed;
    }

    return page_.items.empty()
        ? LibraryRuntimeResult::Empty
        : LibraryRuntimeResult::Applied;
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryRefreshRequested&
) {
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryFilterChanged& event
) {
    app_state_.library.mode = LibraryQueryMode::Browse;
    app_state_.library.filter = event.filter;
    app_state_.library.search_text.clear();
    app_state_.library.offset = 0;
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibrarySortChanged& event
) {
    app_state_.library.sort = event.sort;
    app_state_.library.direction = event.direction;
    app_state_.library.offset = 0;
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibrarySearchChanged& event
) {
    app_state_.library.search_text = event.text;
    app_state_.library.offset = 0;
    app_state_.library.mode =
        event.text.empty()
            ? LibraryQueryMode::Browse
            : LibraryQueryMode::Search;
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryFocusNextRequested&
) {
    if (page_.items.empty()) {
        return LibraryRuntimeResult::Empty;
    }

    const auto current = focusedIndex();
    const std::size_t next =
        current.has_value()
            ? std::min(
                *current + 1U,
                page_.items.size() - 1U
            )
            : 0U;

    if (current.has_value() && next == *current) {
        return LibraryRuntimeResult::Ignored;
    }

    app_state_.library.focused_book =
        page_.items[next].book_id;

    return submitRefresh(
        RefreshReason::FocusChanged,
        RefreshClass::Region
    );
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryFocusPreviousRequested&
) {
    if (page_.items.empty()) {
        return LibraryRuntimeResult::Empty;
    }

    const auto current = focusedIndex();
    const std::size_t previous =
        current.has_value() && *current > 0
            ? *current - 1U
            : 0U;

    if (current.has_value() &&
        previous == *current) {
        return LibraryRuntimeResult::Ignored;
    }

    app_state_.library.focused_book =
        page_.items[previous].book_id;

    return submitRefresh(
        RefreshReason::FocusChanged,
        RefreshClass::Region
    );
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const OpenFocusedBookRequested&
) {
    if (app_state_.screen != Screen::Library ||
        !app_state_.library.focused_book.has_value()) {
        return LibraryRuntimeResult::Ignored;
    }

    const auto result =
        reader_.handle(
            OpenBookRequested{
                *app_state_.library.focused_book,
            }
        );

    return result == ReaderRuntimeResult::Applied
        ? LibraryRuntimeResult::Applied
        : LibraryRuntimeResult::OpenFailed;
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const DeleteFocusedBookRequested&
) {
    if (app_state_.screen != Screen::Library ||
        !app_state_.library.focused_book.has_value() ||
        app_state_.import_active) {
        return LibraryRuntimeResult::Ignored;
    }

    const auto deleting =
        *app_state_.library.focused_book;

    const auto status = deleter_.remove(deleting);
    if (status != BookDeleteStatus::Ok) {
        return LibraryRuntimeResult::DeleteFailed;
    }

    app_state_.library.focused_book.reset();
    return reload(RefreshReason::ScreenChanged);
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const ImportRequested& event
) {
    if (app_state_.import_active) {
        return LibraryRuntimeResult::Ignored;
    }

    app_state_.import_active = true;

    const auto imported = importer_.import(
        event.staged_path,
        event.source_filename,
        event.added_order
    );

    app_state_.import_active = false;
    last_import_status_ = imported.status;

    if (imported.status == StagedImportStatus::Duplicate) {
        return LibraryRuntimeResult::ImportDuplicate;
    }

    if (!imported.ok()) {
        return LibraryRuntimeResult::ImportFailed;
    }

    app_state_.library.mode = LibraryQueryMode::Browse;
    app_state_.library.search_text.clear();
    app_state_.library.offset = 0;
    app_state_.library.focused_book = imported.book_id;

    return reload(RefreshReason::ScreenChanged);
}

} // namespace enku
