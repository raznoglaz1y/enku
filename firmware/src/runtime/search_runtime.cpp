#include "enku/runtime/search_runtime.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

#include "enku/core/events.hpp"

namespace enku {

namespace {

std::string simpleUtf8Fold(
    std::string_view value
) {
    std::string out;
    out.reserve(value.size());

    std::size_t offset = 0;

    while (offset < value.size()) {
        const auto first =
            static_cast<unsigned char>(
                value[offset]
            );

        if (first < 0x80U) {
            char ch =
                static_cast<char>(first);

            if (ch >= 'A' && ch <= 'Z') {
                ch = static_cast<char>(
                    ch - 'A' + 'a'
                );
            }

            out.push_back(ch);
            ++offset;
            continue;
        }

        if (offset + 1U < value.size() &&
            (first & 0xE0U) == 0xC0U) {
            const auto second =
                static_cast<unsigned char>(
                    value[offset + 1U]
                );

            if ((second & 0xC0U) == 0x80U) {
                std::uint32_t codepoint =
                    ((first & 0x1FU) << 6U) |
                    (second & 0x3FU);

                if (codepoint >= 0x0410U &&
                    codepoint <= 0x042FU) {
                    codepoint += 0x20U;
                } else if (codepoint == 0x0401U) {
                    codepoint = 0x0451U;
                }

                out.push_back(
                    static_cast<char>(
                        0xC0U |
                        ((codepoint >> 6U) &
                         0x1FU)
                    )
                );
                out.push_back(
                    static_cast<char>(
                        0x80U |
                        (codepoint & 0x3FU)
                    )
                );
                offset += 2U;
                continue;
            }
        }

        out.push_back(
            static_cast<char>(first)
        );
        ++offset;
    }

    return out;
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

    const auto needle = simpleUtf8Fold(app.search.query);

    for (const auto& section : document->sections) {
        for (const auto& block : section.blocks) {
            const auto haystack = simpleUtf8Fold(block.text);
            std::size_t from = 0;

            while (from < haystack.size()) {
                const auto match =
                    haystack.find(needle, from);
                if (match == std::string::npos) {
                    break;
                }

                ++app.search.total_matches;

                from = match + std::max<std::size_t>(
                    1U,
                    needle.size()
                );
            }
        }
    }

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

    constexpr std::size_t kBatchLimit = 24;
    const auto needle = simpleUtf8Fold(app.search.query);

    app.search.matches.clear();
    app.search.window_start = window_start;

    std::uint32_t global_index = 0;

    for (const auto& section : document->sections) {
        for (const auto& block : section.blocks) {
            const auto haystack = simpleUtf8Fold(block.text);
            std::size_t from = 0;

            while (from < haystack.size()) {
                const auto match =
                    haystack.find(needle, from);

                if (match == std::string::npos) {
                    break;
                }

                if (global_index >= window_start &&
                    app.search.matches.size() <
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

                ++global_index;

                if (app.search.matches.size() >=
                    kBatchLimit) {
                    return SearchRuntimeResult::Applied;
                }

                from = match + std::max<std::size_t>(
                    1U,
                    needle.size()
                );
            }
        }
    }

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
