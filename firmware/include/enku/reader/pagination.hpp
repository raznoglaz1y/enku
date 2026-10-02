#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "document.hpp"
#include "reader_types.hpp"

namespace enku {

enum class PaginationStatus : std::uint8_t {
    Ok,
    EndOfDocument,
    InvalidAnchor,
    ViewportTooSmall,
    MeasurementFailure,
};

struct PaginationPage {
    std::vector<PageLine> lines;
    SemanticPosition first_position;
    SemanticPosition last_position;
    std::optional<SemanticPosition> next_anchor;
    float progress{0.0F};
};

struct PaginationResult {
    PaginationStatus status{PaginationStatus::InvalidAnchor};
    PaginationPage page;

    bool ok() const {
        return status == PaginationStatus::Ok;
    }
};

class TextMeasurer {
public:
    virtual ~TextMeasurer() = default;

    virtual std::uint16_t measureWidthPx(
        std::string_view utf8,
        const TypographySettings& typography
    ) const = 0;

    virtual std::uint16_t lineHeightPx(
        const TypographySettings& typography
    ) const = 0;
};

class TextPaginator {
public:
    PaginationResult paginate(
        const BookDocument& document,
        const LayoutRequest& request,
        const TextMeasurer& measurer
    ) const;
};

} // namespace enku
