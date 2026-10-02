#include "enku/reader/pagination.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace enku {
namespace {

PageLineKind pageLineKind(
    TextBlockType type
) {
    switch (type) {
        case TextBlockType::Heading:
            return PageLineKind::Heading;
        case TextBlockType::Quote:
            return PageLineKind::Quote;
        case TextBlockType::ListItem:
            return PageLineKind::ListItem;
        case TextBlockType::Separator:
            return PageLineKind::Separator;
        case TextBlockType::Paragraph:
        default:
            return PageLineKind::Paragraph;
    }
}

std::uint16_t lineIndent(
    PageLineKind kind
) {
    switch (kind) {
        case PageLineKind::Quote:
            return 18U;
        case PageLineKind::ListItem:
            return 22U;
        default:
            return 0U;
    }
}

bool isContinuation(unsigned char ch) {
    return (ch & 0xC0U) == 0x80U;
}

std::size_t nextCodepoint(std::string_view text, std::size_t index) {
    if (index >= text.size()) {
        return text.size();
    }

    ++index;
    while (index < text.size() &&
           isContinuation(static_cast<unsigned char>(text[index]))) {
        ++index;
    }
    return index;
}

std::size_t skipSpaces(std::string_view text, std::size_t index) {
    while (index < text.size() &&
           (text[index] == ' ' || text[index] == '\t')) {
        ++index;
    }
    return index;
}

std::size_t wordEnd(std::string_view text, std::size_t index) {
    while (index < text.size() &&
           text[index] != ' ' &&
           text[index] != '\t') {
        index = nextCodepoint(text, index);
    }
    return index;
}

struct WrappedLine {
    std::string text;
    std::size_t start{0};
    std::size_t next{0};
    bool valid{false};
};

WrappedLine makeLine(
    std::string_view text,
    std::size_t start,
    std::uint16_t max_width,
    const TypographySettings& typography,
    const TextMeasurer& measurer
) {
    WrappedLine out;
    start = skipSpaces(text, start);

    if (start >= text.size()) {
        out.next = text.size();
        return out;
    }

    const std::size_t line_start = start;
    std::size_t accepted_end = line_start;
    std::size_t cursor = line_start;

    while (cursor < text.size()) {
        const std::size_t end = wordEnd(text, cursor);
        const auto candidate = text.substr(line_start, end - line_start);

        if (measurer.measureWidthPx(candidate, typography) <= max_width) {
            accepted_end = end;
            cursor = skipSpaces(text, end);
            continue;
        }

        break;
    }

    if (accepted_end == line_start) {
        std::size_t end = nextCodepoint(text, line_start);
        std::size_t last_fit = line_start;

        while (end <= text.size()) {
            const auto candidate = text.substr(line_start, end - line_start);
            if (measurer.measureWidthPx(candidate, typography) > max_width) {
                break;
            }

            last_fit = end;
            if (end == text.size()) {
                break;
            }
            end = nextCodepoint(text, end);
        }

        if (last_fit == line_start) {
            return out;
        }

        accepted_end = last_fit;
    }

    std::size_t logical_next = accepted_end;
    logical_next = skipSpaces(text, logical_next);

    out.text = std::string(text.substr(line_start, accepted_end - line_start));
    out.start = line_start;
    out.next = logical_next;
    out.valid = true;
    return out;
}

const DocumentSection* findSection(
    const BookDocument& document,
    std::string_view section_id,
    std::size_t& section_index
) {
    if (document.sections.empty()) {
        return nullptr;
    }

    if (section_id.empty()) {
        section_index = 0;
        return &document.sections[0];
    }

    for (std::size_t i = 0; i < document.sections.size(); ++i) {
        if (document.sections[i].id == section_id) {
            section_index = i;
            return &document.sections[i];
        }
    }

    return nullptr;
}

} // namespace

PaginationResult TextPaginator::paginate(
    const BookDocument& document,
    const LayoutRequest& request,
    const TextMeasurer& measurer
) const {
    PaginationResult result;
    result.page.first_position = request.anchor;
    result.page.last_position = request.anchor;

    const auto margin = request.typography.margin_px;
    if (request.viewport.width <= margin * 2U ||
        request.viewport.height <= margin * 2U) {
        result.status = PaginationStatus::ViewportTooSmall;
        return result;
    }

    const auto content_width =
        static_cast<std::uint16_t>(request.viewport.width - margin * 2U);
    const auto content_height =
        static_cast<std::uint16_t>(request.viewport.height - margin * 2U);

    const auto line_height = measurer.lineHeightPx(request.typography);
    if (line_height == 0 || content_width == 0) {
        result.status = PaginationStatus::MeasurementFailure;
        return result;
    }

    const auto max_lines =
        static_cast<std::uint16_t>(content_height / line_height);
    if (max_lines == 0) {
        result.status = PaginationStatus::ViewportTooSmall;
        return result;
    }

    std::size_t section_index = 0;
    const auto* initial_section =
        findSection(document, request.anchor.section_id, section_index);
    if (initial_section == nullptr) {
        result.status = PaginationStatus::InvalidAnchor;
        return result;
    }

    bool first_line = true;
    std::uint16_t line_index = 0;
    std::uint64_t start_offset = request.anchor.text_offset;

    for (std::size_t s = section_index; s < document.sections.size(); ++s) {
        const auto& section = document.sections[s];

        for (const auto& block : section.blocks) {
            const auto block_start = block.text_offset;
            const auto block_end = block.text_offset + block.text.size();
            const auto kind =
                pageLineKind(block.type);

            if (s == section_index && start_offset >= block_end) {
                continue;
            }

            std::size_t local =
                (s == section_index && start_offset > block_start)
                    ? static_cast<std::size_t>(start_offset - block_start)
                    : 0U;

            if (local > block.text.size()) {
                continue;
            }

            // Give structural headings one blank line before them when they
            // don't start the page. This keeps EPUB/FB2 chapter transitions
            // readable without changing the base typography scale.
            if (kind == PageLineKind::Heading &&
                local == 0U &&
                line_index > 0U) {
                if (line_index + 1U >= max_lines) {
                    result.page.next_anchor = SemanticPosition{
                        document.book_id,
                        section.id,
                        block_start,
                    };
                    result.status = PaginationStatus::Ok;
                    const auto absolute =
                        result.page.last_position.text_offset;
                    result.page.progress =
                        document.total_text_length == 0
                            ? 0.0F
                            : std::min(
                                1.0F,
                                static_cast<float>(absolute) /
                                    static_cast<float>(
                                        document.total_text_length
                                    )
                            );
                    return result;
                }
                ++line_index;
            }

            const auto indent =
                lineIndent(kind);
            const auto line_width =
                content_width > indent
                    ? static_cast<std::uint16_t>(
                        content_width - indent
                    )
                    : 0U;

            if (line_width == 0U) {
                result.status =
                    PaginationStatus::ViewportTooSmall;
                return result;
            }

            while (local < block.text.size()) {
                if (line_index >= max_lines) {
                    result.page.next_anchor = SemanticPosition{
                        document.book_id,
                        section.id,
                        block_start + local,
                    };
                    result.status = PaginationStatus::Ok;
                    const auto absolute = result.page.last_position.text_offset;
                    result.page.progress = document.total_text_length == 0
                        ? 0.0F
                        : std::min(
                            1.0F,
                            static_cast<float>(absolute) /
                                static_cast<float>(document.total_text_length)
                        );
                    return result;
                }

                const auto wrapped = makeLine(
                    block.text,
                    local,
                    line_width,
                    request.typography,
                    measurer
                );

                if (!wrapped.valid) {
                    result.status = PaginationStatus::MeasurementFailure;
                    return result;
                }

                PageLine line;
                line.text = wrapped.text;
                line.position = SemanticPosition{
                    document.book_id,
                    section.id,
                    block_start + wrapped.start,
                };
                line.x =
                    static_cast<std::uint16_t>(
                        margin + indent
                    );
                line.y = static_cast<std::uint16_t>(
                    margin + line_index * line_height
                );
                line.kind = kind;

                if (first_line) {
                    result.page.first_position = line.position;
                    first_line = false;
                }

                result.page.last_position = SemanticPosition{
                    document.book_id,
                    section.id,
                    block_start + wrapped.next,
                };
                result.page.lines.push_back(std::move(line));
                ++line_index;
                local = wrapped.next;
            }

            start_offset = 0;
        }

        start_offset = 0;
    }

    if (result.page.lines.empty()) {
        result.status = PaginationStatus::EndOfDocument;
        return result;
    }

    result.status = PaginationStatus::Ok;
    result.page.next_anchor.reset();
    result.page.progress = 1.0F;
    return result;
}

} // namespace enku
