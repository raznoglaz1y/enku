#include "enku/runtime/library_search_runtime.hpp"

namespace enku {

LibrarySearchRuntime::LibrarySearchRuntime(
    AppState& app_state,
    LibraryRuntimeController& library
)
    : app_state_(app_state),
      library_(library),
      keyboard_(app_state.keyboard) {}

LibrarySearchRuntimeResult
LibrarySearchRuntime::open() {
    if (app_state_.screen != Screen::Library) {
        return LibrarySearchRuntimeResult::Ignored;
    }

    if (app_state_.library.mode !=
        LibraryQueryMode::Search) {
        origin_valid_ = true;
        origin_offset_ =
            app_state_.library.offset;
        origin_focused_book_ =
            app_state_.library.focused_book;
    }

    app_state_.library.mode =
        LibraryQueryMode::Search;
    keyboard_.open();

    const auto redraw =
        library_.redraw();

    return redraw == LibraryRuntimeResult::Applied
        ? LibrarySearchRuntimeResult::Applied
        : LibrarySearchRuntimeResult::Failed;
}

LibrarySearchRuntimeResult
LibrarySearchRuntime::handle(
    const OpenLibrarySearchRequested&
) {
    return open();
}

LibrarySearchRuntimeResult
LibrarySearchRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::Library ||
        app_state_.library.mode !=
            LibraryQueryMode::Search) {
        return LibrarySearchRuntimeResult::Ignored;
    }

    if (app_state_.keyboard.open) {
        const auto result =
            keyboard_.handle(
                action,
                app_state_.library.search_text
            );

        if (result == KeyboardRuntimeResult::Done) {
            return applyQuery();
        }

        if (result == KeyboardRuntimeResult::Changed) {
            const auto redraw =
                library_.redraw(
                    RefreshReason::FocusChanged,
                    RefreshClass::Region
                );

            return redraw == LibraryRuntimeResult::Applied
                ? LibrarySearchRuntimeResult::Applied
                : LibrarySearchRuntimeResult::Failed;
        }

        if (result == KeyboardRuntimeResult::Closed) {
            return cancel();
        }

        return LibrarySearchRuntimeResult::Ignored;
    }

    if (action == LogicalAction::Confirm) {
        keyboard_.open(
            app_state_.keyboard.mode
        );

        const auto redraw =
            library_.redraw();

        return redraw == LibraryRuntimeResult::Applied
            ? LibrarySearchRuntimeResult::Applied
            : LibrarySearchRuntimeResult::Failed;
    }

    if (action == LogicalAction::Back) {
        return cancel();
    }

    return LibrarySearchRuntimeResult::Ignored;
}

LibrarySearchRuntimeResult
LibrarySearchRuntime::submit() {
    if (app_state_.screen != Screen::Library ||
        app_state_.library.mode !=
            LibraryQueryMode::Search) {
        return LibrarySearchRuntimeResult::Ignored;
    }

    return applyQuery();
}

LibrarySearchRuntimeResult
LibrarySearchRuntime::applyQuery() {
    app_state_.keyboard.open = false;

    const auto result =
        library_.handle(
            LibrarySearchChanged{
                app_state_.library.search_text
            }
        );

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? LibrarySearchRuntimeResult::Applied
        : LibrarySearchRuntimeResult::Failed;
}

LibrarySearchRuntimeResult
LibrarySearchRuntime::cancel() {
    app_state_.keyboard = KeyboardState{};
    app_state_.library.search_text.clear();
    app_state_.library.mode =
        LibraryQueryMode::Browse;

    if (origin_valid_) {
        app_state_.library.offset =
            origin_offset_;
        app_state_.library.focused_book =
            origin_focused_book_;
    } else {
        app_state_.library.offset = 0;
        app_state_.library.focused_book.reset();
    }

    const auto result =
        library_.handle(
            LibraryRefreshRequested{}
        );

    origin_valid_ = false;
    origin_offset_ = 0;
    origin_focused_book_.reset();

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? LibrarySearchRuntimeResult::Applied
        : LibrarySearchRuntimeResult::Failed;
}

} // namespace enku
