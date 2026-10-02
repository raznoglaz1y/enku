#pragma once

#include <cstdint>
#include <optional>

#include "../core/types.hpp"

namespace enku {

struct TypographySettings {
    std::uint16_t font_size_px{0};
    float line_spacing{1.0F};
    std::uint16_t margin_px{0};
};

struct LayoutRequest {
    BookId book_id;
    SemanticPosition anchor;
    TypographySettings typography;
    Viewport viewport;
};

struct PageResult {
    SemanticPosition first_position;
    SemanticPosition last_position;
    std::optional<SemanticPosition> previous_anchor;
    std::optional<SemanticPosition> next_anchor;
    float progress{0.0F};

    // Rendering payload/display-list representation is intentionally
    // left implementation-defined until the font/raster stack is selected.
};

} // namespace enku
