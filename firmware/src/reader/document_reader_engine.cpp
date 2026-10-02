#include "enku/reader/document_reader_engine.hpp"

#include <utility>

namespace enku {

DocumentReaderEngine::DocumentReaderEngine(
    const BookDocument& document,
    const TextMeasurer& measurer
)
    : document_(document),
      measurer_(measurer) {}

std::optional<PageResult> DocumentReaderEngine::layoutPage(
    const LayoutRequest& request
) {
    if (request.book_id != document_.book_id ||
        request.anchor.book_id != document_.book_id) {
        return std::nullopt;
    }

    const auto pagination =
        paginator_.paginate(document_, request, measurer_);

    if (!pagination.ok()) {
        return std::nullopt;
    }

    PageResult result;
    result.lines = pagination.page.lines;
    result.first_position = pagination.page.first_position;
    result.last_position = pagination.page.last_position;
    result.next_anchor = pagination.page.next_anchor;
    result.progress = pagination.page.progress;

    // Deterministic previous-page reconstruction is intentionally
    // not guessed here. It will be supplied by the Reader session/cache layer.
    result.previous_anchor.reset();

    return result;
}

} // namespace enku
