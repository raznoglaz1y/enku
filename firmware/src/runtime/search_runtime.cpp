#include "enku/runtime/search_runtime.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "enku/core/events.hpp"

namespace enku {

namespace {

std::string asciiFold(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );
    return value;
}

std::string previewAround(
    const std::string& text,
    std::size_t match
) {
    constexpr std::size_t kRadius = 36;
    const auto start =
        match > kRadius ? match - kRadius : 0U;
    const auto count =
        std::min<std::size_t>(
            text.size() - start,
            kRadius * 2U
        );
    return text.substr(start, count);
}

} // namespace

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
        return cancel();
    }

    if (action == LogicalAction::Confirm &&
        app.search.phase == SearchPhase::QueryEntry) {
        keyboard_.open();
        return render();
    }

    if (app.search.phase == SearchPhase::Results) {
        if (action == LogicalAction::NavigatePrevious &&
            app.search.focus_index > 0U) {
            --app.search.focus_index;
            return render();
        }

        if (action == LogicalAction::NavigateNext &&
            app.search.focus_index + 1U <
                app.search.matches.size()) {
            ++app.search.focus_index;
            return render();
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

    if (app.search.query.empty()) {
        app.search.phase = SearchPhase::QueryEntry;
        return render();
    }

    const auto* document = reader_.loader().document();
    if (document == nullptr) {
        return SearchRuntimeResult::Failed;
    }

    const auto needle = asciiFold(app.search.query);
    constexpr std::size_t kBatchLimit = 24;

    for (const auto& section : document->sections) {
        for (const auto& block : section.blocks) {
            const auto haystack = asciiFold(block.text);
            std::size_t from = 0;

            while (from < haystack.size()) {
                const auto match =
                    haystack.find(needle, from);
                if (match == std::string::npos) {
                    break;
                }

                ++app.search.total_matches;

                if (app.search.matches.size() <
                    kBatchLimit) {
                    app.search.matches.push_back(
                        SearchMatch{
                            SemanticPosition{
                                document->book_id,
                                section.id,
                                block.text_offset +
                                    static_cast<std::uint64_t>(
                                        match
                                    ),
                            },
                            previewAround(
                                block.text,
                                match
                            ),
                        }
                    );
                }

                from = match + std::max<std::size_t>(
                    1U,
                    needle.size()
                );
            }
        }
    }

    app.search.phase = SearchPhase::Results;
    return render();
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
