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
    SettingsRuntimeController& settings,
    RefreshService& refresh,
    LibraryPageRenderer* page_renderer
)
    : app_state_(app_state),
      library_(library),
      reader_(reader),
      importer_(importer),
      deleter_(deleter),
      settings_(settings),
      refresh_(refresh),
      page_renderer_(page_renderer) {}

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

void LibraryRuntimeController::resetQueryWindow() {
    app_state_.library.offset = 0;
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

LibraryRuntimeResult LibraryRuntimeController::redraw(
    RefreshReason reason,
    RefreshClass refresh_class
) {
    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
    }

    return submitRefresh(
        reason,
        refresh_class
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

    if (next_page.items.empty() &&
        next_page.total_matches > 0U &&
        app_state_.library.offset > 0U) {
        const auto limit =
            static_cast<std::uint32_t>(
                app_state_.library.limit
            );

        const auto last_offset =
            ((next_page.total_matches - 1U) /
             limit) *
            limit;

        app_state_.library.offset =
            last_offset;

        LibraryPage clamped_page;
        const auto clamped_status =
            library_.query(
                queryFromState(),
                clamped_page
            );

        if (clamped_status != LibraryStatus::Ok) {
            return LibraryRuntimeResult::QueryFailed;
        }

        next_page = std::move(clamped_page);
    }

    page_ = std::move(next_page);
    normalizeFocus();

    if (app_state_.screen != Screen::Library) {
        app_state_.screen = Screen::Library;
    }

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
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
    if (settings_.handle(event) != PersistStatus::Ok) {
        return LibraryRuntimeResult::SettingsSaveFailed;
    }
    app_state_.library.search_text.clear();
    resetQueryWindow();
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibrarySortChanged& event
) {
    if (settings_.handle(event) != PersistStatus::Ok) {
        return LibraryRuntimeResult::SettingsSaveFailed;
    }
    resetQueryWindow();
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryViewChanged& event
) {
    if (settings_.handle(event) != PersistStatus::Ok) {
        return LibraryRuntimeResult::SettingsSaveFailed;
    }

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
    }

    return submitRefresh(
        RefreshReason::ScreenChanged,
        RefreshClass::Full
    );
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibrarySearchChanged& event
) {
    app_state_.library.search_text = event.text;
    resetQueryWindow();
    app_state_.library.mode =
        event.text.empty()
            ? LibraryQueryMode::Browse
            : LibraryQueryMode::Search;
    return reload();
}

LibraryRuntimeResult LibraryRuntimeController::moveToOffset(
    std::uint32_t offset,
    bool focus_last
) {
    app_state_.library.offset = offset;

    LibraryPage next_page;
    const auto status =
        library_.query(
            queryFromState(),
            next_page
        );

    if (status != LibraryStatus::Ok) {
        return LibraryRuntimeResult::QueryFailed;
    }

    page_ = std::move(next_page);
    app_state_.library.total_matches =
        page_.total_matches;

    if (page_.items.empty()) {
        app_state_.library.focused_book.reset();
    } else {
        app_state_.library.focused_book =
            focus_last
                ? page_.items.back().book_id
                : page_.items.front().book_id;
    }

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
    }

    return submitRefresh(
        RefreshReason::FocusChanged,
        RefreshClass::Full
    );
}

LibraryRuntimeResult LibraryRuntimeController::revealBook(
    const BookId& book_id,
    RefreshReason reason
) {
    const auto limit =
        static_cast<std::uint32_t>(
            app_state_.library.limit
        );

    if (limit == 0U) {
        return LibraryRuntimeResult::QueryFailed;
    }

    std::uint32_t offset = 0U;

    while (true) {
        app_state_.library.offset = offset;

        LibraryPage candidate;
        const auto status =
            library_.query(
                queryFromState(),
                candidate
            );

        if (status != LibraryStatus::Ok) {
            return LibraryRuntimeResult::QueryFailed;
        }

        const auto found = std::find_if(
            candidate.items.begin(),
            candidate.items.end(),
            [&](const BookRecord& record) {
                return record.book_id == book_id;
            }
        );

        if (found != candidate.items.end()) {
            page_ = std::move(candidate);
            app_state_.library.total_matches =
                page_.total_matches;
            app_state_.library.focused_book =
                book_id;

            if (page_renderer_ != nullptr &&
                !page_renderer_->renderLibrary(
                    app_state_,
                    page_
                )) {
                return LibraryRuntimeResult::RenderFailed;
            }

            return submitRefresh(
                reason,
                RefreshClass::Full
            );
        }

        const auto next_offset =
            offset +
            static_cast<std::uint32_t>(
                candidate.items.size()
            );

        if (candidate.items.empty() ||
            next_offset >= candidate.total_matches) {
            break;
        }

        offset = next_offset;
    }

    resetQueryWindow();
    app_state_.library.focused_book.reset();
    return reload(reason);
}

LibraryRuntimeResult LibraryRuntimeController::handle(
    const LibraryFocusNextRequested&
) {
    if (page_.items.empty()) {
        return LibraryRuntimeResult::Empty;
    }

    const auto current = focusedIndex();

    if (current.has_value() &&
        *current + 1U >= page_.items.size()) {
        const auto next_offset =
            page_.offset +
            static_cast<std::uint32_t>(
                page_.items.size()
            );

        if (next_offset <
            page_.total_matches) {
            return moveToOffset(
                next_offset,
                false
            );
        }

        return LibraryRuntimeResult::Ignored;
    }

    const std::size_t next =
        current.has_value()
            ? *current + 1U
            : 0U;

    app_state_.library.focused_book =
        page_.items[next].book_id;

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
    }

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

    if (current.has_value() &&
        *current == 0U) {
        if (page_.offset == 0U) {
            return LibraryRuntimeResult::Ignored;
        }

        const auto step =
            static_cast<std::uint32_t>(
                app_state_.library.limit
            );
        const auto previous_offset =
            page_.offset > step
                ? page_.offset - step
                : 0U;

        return moveToOffset(
            previous_offset,
            true
        );
    }

    const std::size_t previous =
        current.has_value()
            ? *current - 1U
            : 0U;

    app_state_.library.focused_book =
        page_.items[previous].book_id;

    if (page_renderer_ != nullptr &&
        !page_renderer_->renderLibrary(
            app_state_,
            page_
        )) {
        return LibraryRuntimeResult::RenderFailed;
    }

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

    return revealBook(
        imported.book_id,
        RefreshReason::ScreenChanged
    );
}

} // namespace enku
