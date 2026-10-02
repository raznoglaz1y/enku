#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../core/library.hpp"

namespace enku {

enum class TextBlockType : std::uint8_t {
    Paragraph,
    Heading,
    Quote,
    ListItem,
    Separator,
};

struct TextBlock {
    TextBlockType type{TextBlockType::Paragraph};
    std::string text;
    std::uint64_t text_offset{0};
};

struct DocumentSection {
    std::string id;
    std::optional<std::string> title;
    std::vector<TextBlock> blocks;
    std::uint64_t text_length{0};
};

struct BookDocument {
    BookId book_id;
    BookMetadata metadata;
    std::vector<DocumentSection> sections;
    std::uint64_t total_text_length{0};
};

} // namespace enku
