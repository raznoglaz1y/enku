#include "enku/runtime/search_runtime.hpp"

#include <string>
#include <utility>

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
      refresh_(refresh),
      keyboard_(storage_.appState().keyboard) {}

SearchRuntimeResult SearchRuntime::openFromReader() {
    auto& app = storage_.appState();

    if (app.screen != Screen::Reading ||
        !app.reading_position.has_value()) {
        return SearchRuntimeResult::Ignored;
    }

    app.search = ReaderSearchState{};
    app.search.origin_position = app.reading_position;
    app.search.phase = SearchPhase::QueryEntry;
    app.keyboard = KeyboardState{};
    app.screen = Screen::Search;

    return render();
}

SearchRuntimeResult SearchRuntime::submitQuery(
    std::string query
) {
    auto& app = storage_.appState();

    if (app.screen != Screen::Search) {
        return SearchRuntimeResult::Ignored;
    }

    app.search.query = std::move(query);
    app.keyboard.open = false;
    return executeSearch();
}

SearchRuntimeResult SearchRuntime::handle(
    LogicalAction action
) {
    auto& app = storage_.appState();

    if (app.screen != Screen::Search) {
        return SearchRuntimeResult::Ignored;
    }

    if (app.keyboard.open) {
        return handleKeyboard(action);
    }

    if (action == LogicalAction::Back) {
        if (app.search.phase == SearchPhase::Results ||
            app.search.phase == SearchPhase::NoResults) {
            app.search.phase = SearchPhase::QueryEntry;
            app.search.matches.clear();
            app.search.total_matches = 0;
            app.search.focus_index = 0;
            app.search.window_start = 0;
            return render();
        }

        return cancel();
    }

    if (action == LogicalAction::Confirm &&
        (app.search.phase == SearchPhase::QueryEntry ||
         app.search.phase == SearchPhase::NoResults)) {
        keyboard_.open();
        return render();
    }

    if (app.search.phase == SearchPhase::Results) {
        if (action == LogicalAction::NavigatePrevious) {
            if (app.search.focus_index > 0U) {
                --app.search.focus_index;
                return render();
            }

            if (app.search.window_start > 0U) {
                const auto new_start =
                    app.search.window_start >= 24U
                        ? app.search.window_start - 24U
                        : 0U;

                const auto result =
                    populateWindow(new_start);

                if (result != SearchRuntimeResult::Applied) {
                    return result;
                }

                if (!app.search.matches.empty()) {
                    app.search.focus_index =
                        static_cast<std::uint32_t>(
                            app.search.matches.size() - 1U
                        );
                }

                return render();
            }
        }

        if (action == LogicalAction::NavigateNext) {
            if (app.search.focus_index + 1U <
                app.search.matches.size()) {
                ++app.search.focus_index;
                return render();
            }

            const auto next_global =
                app.search.window_start +
                static_cast<std::uint32_t>(
                    app.search.matches.size()
                );

            if (next_global < app.search.total_matches) {
                const auto result =
                    populateWindow(next_global);

                if (result != SearchRuntimeResult::Applied) {
                    return result;
                }

                app.search.focus_index = 0;
                return render();
            }
        }

        if (action == LogicalAction::Confirm &&
            !app.search.matches.empty() &&
            app.search.focus_index <
                app.search.matches.size()) {
            const auto selected =
                app.search.matches[
                    app.search.focus_index
                ];

            app.search_highlight.position =
                selected.position;
            app.search_highlight.query =
                app.search.query;

            const auto jump =
                reader_.reader().handle(
                    BookPositionChanged{
                        selected.position
                    }
                );

            if (jump != ReaderRuntimeResult::Applied) {
                app.search_highlight =
                    ReaderSearchHighlight{};
                return SearchRuntimeResult::Failed;
            }

            app.search = ReaderSearchState{};
            app.keyboard = KeyboardState{};
            return SearchRuntimeResult::Applied;
        }
    }

    return SearchRuntimeResult::Ignored;
}

SearchRuntimeResult SearchRuntime::handleKeyboard(
    LogicalAction action
) {
    auto& app = storage_.appState();
    const auto result =
        keyboard_.handle(action, app.search.query);

    if (result == KeyboardRuntimeResult::Done) {
        return executeSearch();
    }

    if (result == KeyboardRuntimeResult::Changed ||
        result == KeyboardRuntimeResult::Closed) {
        return render();
    }

    return SearchRuntimeResult::Ignored;
}

SearchRuntimeResult SearchRuntime::executeSearch() {
    auto& app = storage_.appState();
    app.search.matches.clear();
    app.search.total_matches = 0;
    app.search.focus_index = 0;
    app.search.window_start = 0;

    if (app.search.query.empty()) {
        app.search.phase = SearchPhase::QueryEntry;
        return render();
    }

    const auto* document = reader_.loader().document();
    if (document == nullptr) {
        return SearchRuntimeResult::Failed;
    }

    app.search.total_matches =
        document_search_.count(
            *document,
            app.search.query
        );

    if (app.search.total_matches == 0U) {
        app.search.phase = SearchPhase::NoResults;
        return render();
    }

    app.search.phase = SearchPhase::Results;

    const auto window_result =
        populateWindow(0U);

    if (window_result != SearchRuntimeResult::Applied) {
        return window_result;
    }

    return render();
}

SearchRuntimeResult SearchRuntime::populateWindow(
    std::uint32_t window_start
) {
    auto& app = storage_.appState();
    const auto* document = reader_.loader().document();

    if (document == nullptr || app.search.query.empty()) {
        return SearchRuntimeResult::Failed;
    }

    constexpr std::uint32_t kBatchLimit = 24U;
    const auto result =
        document_search_.window(
            *document,
            app.search.query,
            window_start,
            kBatchLimit
        );

    app.search.matches.clear();
    app.search.matches.reserve(result.matches.size());

    for (const auto& match : result.matches) {
        app.search.matches.push_back(
            SearchMatch{
                match.position,
                match.section_label,
                match.preview,
                match.preview_match_start,
                match.preview_match_length,
            }
        );
    }

    app.search.total_matches =
        result.total_matches;
    app.search.window_start =
        result.window_start;

    return SearchRuntimeResult::Applied;
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
    app.search_highlight = ReaderSearchHighlight{};
    app.keyboard = KeyboardState{};
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
