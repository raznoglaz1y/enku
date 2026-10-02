#include "enku/runtime/search_runtime.hpp"

#include "enku/core/events.hpp"

namespace enku {

SearchRuntime::SearchRuntime(
    ApplicationStorageRuntime& storage,
    ApplicationReaderRuntime& reader,
    SearchRenderer* renderer,
    RefreshService* refresh
)
    : storage_(storage),
      reader_(reader),
      renderer_(renderer),
      refresh_(refresh) {}

SearchRuntimeResult SearchRuntime::openFromReader() {
    auto& app = storage_.appState();

    if (app.screen != Screen::Reading ||
        !app.reading_position.has_value()) {
        return SearchRuntimeResult::Ignored;
    }

    app.search.origin_position = app.reading_position;
    app.search.query.clear();
    app.search.total_matches = 0;
    app.search.focus_index = 0;
    app.search.phase = SearchPhase::QueryEntry;
    app.screen = Screen::Search;

    return render();
}

SearchRuntimeResult SearchRuntime::handle(
    LogicalAction action
) {
    if (storage_.appState().screen != Screen::Search) {
        return SearchRuntimeResult::Ignored;
    }

    if (action == LogicalAction::Back) {
        return cancel();
    }

    return SearchRuntimeResult::Ignored;
}

SearchRuntimeResult SearchRuntime::cancel() {
    auto& app = storage_.appState();

    if (!app.search.origin_position.has_value()) {
        return SearchRuntimeResult::Failed;
    }

    app.reading_position = app.search.origin_position;
    app.screen = Screen::Reading;

    const auto current =
        storage_.settingsRuntime().current();

    const auto redraw =
        reader_.reader().handle(
            TypographyDefaultsChanged{
                current.reading_preset,
                current.font_size_px,
                current.line_spacing,
                current.margin_px,
            }
        );

    if (redraw != ReaderRuntimeResult::Applied) {
        app.screen = Screen::Search;
        return SearchRuntimeResult::Failed;
    }

    app.search = ReaderSearchState{};
    return SearchRuntimeResult::Applied;
}

SearchRuntimeResult SearchRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderSearch(storage_.appState())) {
        return SearchRuntimeResult::Failed;
    }

    return refreshCurrentFrame();
}

SearchRuntimeResult SearchRuntime::refreshCurrentFrame() {
    if (refresh_ == nullptr) {
        return SearchRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::OverlayChanged;
    request.generation = 0;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? SearchRuntimeResult::Applied
        : SearchRuntimeResult::Failed;
}

} // namespace enku
