#pragma once

#include "document.hpp"
#include "pagination.hpp"
#include "reader_engine.hpp"

namespace enku {

class DocumentReaderEngine final : public ReaderEngine {
public:
    DocumentReaderEngine(
        const BookDocument& document,
        const TextMeasurer& measurer
    );

    std::optional<PageResult> layoutPage(
        const LayoutRequest& request
    ) override;

private:
    const BookDocument& document_;
    const TextMeasurer& measurer_;
    TextPaginator paginator_;
};

} // namespace enku
