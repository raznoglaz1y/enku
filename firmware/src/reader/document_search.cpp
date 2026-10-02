#include "enku/reader/document_search.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

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

bool isUtf8Continuation(unsigned char ch) {
    return (ch & 0xC0U) == 0x80U;
}

std::size_t clampUtf8Start(
    const std::string& text,
    std::size_t offset
) {
    offset = std::min(offset, text.size());

    while (offset > 0U &&
           offset < text.size() &&
           isUtf8Continuation(
               static_cast<unsigned char>(
                   text[offset]
               )
           )) {
        --offset;
    }

    return offset;
}

std::size_t clampUtf8End(
    const std::string& text,
    std::size_t offset
) {
    offset = std::min(offset, text.size());

    while (offset < text.size() &&
           isUtf8Continuation(
               static_cast<unsigned char>(
                   text[offset]
               )
           )) {
        ++offset;
    }

    return offset;
}

DocumentSearchMatch makeMatch(
    const BookDocument& document,
    const DocumentSection& section,
    const TextBlock& block,
    std::size_t match,
    std::size_t match_length
) {
    constexpr std::size_t kRadius = 40;

    auto start =
        match > kRadius
            ? match - kRadius
            : 0U;
    auto end =
        std::min<std::size_t>(
            block.text.size(),
            match + match_length + kRadius
        );

    start = clampUtf8Start(block.text, start);
    end = clampUtf8End(block.text, end);

    DocumentSearchMatch result;
    result.position = SemanticPosition{
        document.book_id,
        section.id,
        block.text_offset +
            static_cast<std::uint64_t>(match),
    };
    result.section_label =
        section.title.has_value()
            ? *section.title
            : section.id;
    result.preview =
        block.text.substr(start, end - start);
    result.preview_match_start =
        static_cast<std::uint32_t>(
            match - start
        );
    result.preview_match_length =
        static_cast<std::uint32_t>(
            match_length
        );
    return result;
}

} // namespace

std::uint32_t DocumentSearch::count(
    const BookDocument& document,
    const std::string& query
) const {
    if (query.empty()) {
        return 0;
    }

    const auto needle = simpleUtf8Fold(query);
    std::uint32_t total = 0;

    for (const auto& section : document.sections) {
        for (const auto& block : section.blocks) {
            const auto haystack =
                simpleUtf8Fold(block.text);
            std::size_t from = 0;

            while (from < haystack.size()) {
                const auto match =
                    haystack.find(needle, from);

                if (match == std::string::npos) {
                    break;
                }

                ++total;
                from = match +
                    std::max<std::size_t>(
                        1U,
                        needle.size()
                    );
            }
        }
    }

    return total;
}

DocumentSearchResult DocumentSearch::window(
    const BookDocument& document,
    const std::string& query,
    std::uint32_t window_start,
    std::uint32_t limit
) const {
    DocumentSearchResult result;
    result.total_matches =
        count(document, query);
    result.window_start = window_start;

    if (query.empty() || limit == 0U ||
        window_start >= result.total_matches) {
        return result;
    }

    const auto needle = simpleUtf8Fold(query);
    std::uint32_t global_index = 0;

    for (const auto& section : document.sections) {
        for (const auto& block : section.blocks) {
            const auto haystack =
                simpleUtf8Fold(block.text);
            std::size_t from = 0;

            while (from < haystack.size()) {
                const auto match =
                    haystack.find(needle, from);

                if (match == std::string::npos) {
                    break;
                }

                if (global_index >= window_start) {
                    result.matches.push_back(
                        makeMatch(
                            document,
                            section,
                            block,
                            match,
                            needle.size()
                        )
                    );

                    if (result.matches.size() >= limit) {
                        return result;
                    }
                }

                ++global_index;
                from = match +
                    std::max<std::size_t>(
                        1U,
                        needle.size()
                    );
            }
        }
    }

    return result;
}

} // namespace enku
