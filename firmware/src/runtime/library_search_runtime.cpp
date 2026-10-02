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

    app_state_.library.mode =
        LibraryQueryMode::Search;
    keyboard_.open();

    return LibrarySearchRuntimeResult::Applied;
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
            return LibrarySearchRuntimeResult::Applied;
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
        return LibrarySearchRuntimeResult::Applied;
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

    const auto result =
        library_.handle(
            LibrarySearchChanged{""}
        );

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? LibrarySearchRuntimeResult::Applied
        : LibrarySearchRuntimeResult::Failed;
}

} // namespace enku
