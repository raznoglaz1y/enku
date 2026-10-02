#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

enum class PageLineKind : std::uint8_t {
    Paragraph,
    Heading,
    Quote,
    ListItem,
    Separator,
};

struct PageLine {
    std::string text;
    SemanticPosition position;
    std::uint16_t x{0};
    std::uint16_t y{0};
    PageLineKind kind{PageLineKind::Paragraph};
};

struct PageResult {
    std::vector<PageLine> lines;
    SemanticPosition first_position;
    SemanticPosition last_position;
    std::optional<SemanticPosition> previous_anchor;
    std::optional<SemanticPosition> next_anchor;
    float progress{0.0F};
};

} // namespace enku
