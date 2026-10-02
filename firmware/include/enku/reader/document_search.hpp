#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "document.hpp"

namespace enku {

struct DocumentSearchMatch {
    SemanticPosition position;
    std::string section_label;
    std::string preview;
    std::uint32_t preview_match_start{0};
    std::uint32_t preview_match_length{0};
};

struct DocumentSearchResult {
    std::uint32_t total_matches{0};
    std::uint32_t window_start{0};
    std::vector<DocumentSearchMatch> matches;
};

class DocumentSearch {
public:
    std::uint32_t count(
        const BookDocument& document,
        const std::string& query
    ) const;

    DocumentSearchResult window(
        const BookDocument& document,
        const std::string& query,
        std::uint32_t window_start,
        std::uint32_t limit
    ) const;
};

} // namespace enku
