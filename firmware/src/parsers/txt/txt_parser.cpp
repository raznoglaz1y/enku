#include "enku/reader/txt_parser.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace enku {
namespace {

bool isContinuation(unsigned char ch) {
    return (ch & 0xC0U) == 0x80U;
}

bool validUtf8(std::string_view text) {
    std::size_t i = 0;

    while (i < text.size()) {
        const auto c = static_cast<unsigned char>(text[i]);

        if (c <= 0x7FU) {
            ++i;
            continue;
        }

        std::size_t continuation_count = 0;
        std::uint32_t code_point = 0;

        if ((c & 0xE0U) == 0xC0U) {
            continuation_count = 1;
            code_point = c & 0x1FU;
            if (code_point == 0) {
                return false; // overlong 2-byte sequence
            }
        } else if ((c & 0xF0U) == 0xE0U) {
            continuation_count = 2;
            code_point = c & 0x0FU;
        } else if ((c & 0xF8U) == 0xF0U) {
            continuation_count = 3;
            code_point = c & 0x07U;
        } else {
            return false;
        }

        if (i + continuation_count >= text.size()) {
            return false;
        }

        for (std::size_t n = 1; n <= continuation_count; ++n) {
            const auto next = static_cast<unsigned char>(text[i + n]);
            if (!isContinuation(next)) {
                return false;
            }
            code_point = (code_point << 6U) | (next & 0x3FU);
        }

        if ((continuation_count == 1 && code_point < 0x80U) ||
            (continuation_count == 2 && code_point < 0x800U) ||
            (continuation_count == 3 && code_point < 0x10000U) ||
            code_point > 0x10FFFFU ||
            (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
            return false;
        }

        i += continuation_count + 1;
    }

    return true;
}

std::string stripUtf8Bom(std::string_view bytes) {
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEFU &&
        static_cast<unsigned char>(bytes[1]) == 0xBBU &&
        static_cast<unsigned char>(bytes[2]) == 0xBFU) {
        return std::string(bytes.substr(3));
    }

    return std::string(bytes);
}

bool hasUtf16Bom(std::string_view bytes) {
    if (bytes.size() < 2) {
        return false;
    }

    const auto b0 = static_cast<unsigned char>(bytes[0]);
    const auto b1 = static_cast<unsigned char>(bytes[1]);

    return (b0 == 0xFFU && b1 == 0xFEU) ||
           (b0 == 0xFEU && b1 == 0xFFU);
}

std::string normalizeLineEndings(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            out.push_back('\n');
        } else {
            out.push_back(text[i]);
        }
    }

    return out;
}

std::string trimAsciiWhitespace(std::string_view text) {
    std::size_t first = 0;
    while (first < text.size() &&
           (text[first] == ' ' || text[first] == '\t' || text[first] == '\n')) {
        ++first;
    }

    std::size_t last = text.size();
    while (last > first &&
           (text[last - 1] == ' ' || text[last - 1] == '\t' || text[last - 1] == '\n')) {
        --last;
    }

    return std::string(text.substr(first, last - first));
}

std::string filenameStem(std::string_view filename) {
    const auto slash = filename.find_last_of("/\\");
    const auto base = slash == std::string_view::npos
        ? filename
        : filename.substr(slash + 1);

    const auto dot = base.find_last_of('.');
    const auto stem = dot == std::string_view::npos
        ? base
        : base.substr(0, dot);

    const auto trimmed = trimAsciiWhitespace(stem);
    return trimmed.empty() ? std::string("Untitled book") : trimmed;
}

std::vector<std::string> splitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t start = 0;

    while (start <= text.size()) {
        const auto end = text.find('\n', start);
        if (end == std::string_view::npos) {
            lines.emplace_back(text.substr(start));
            break;
        }

        lines.emplace_back(text.substr(start, end - start));
        start = end + 1;

        if (start == text.size()) {
            lines.emplace_back();
            break;
        }
    }

    return lines;
}

void flushParagraph(
    std::string& paragraph,
    DocumentSection& section,
    std::uint64_t& normalized_offset
) {
    const auto clean = trimAsciiWhitespace(paragraph);
    paragraph.clear();

    if (clean.empty()) {
        return;
    }

    TextBlock block;
    block.type = TextBlockType::Paragraph;
    block.text = clean;
    block.text_offset = normalized_offset;

    normalized_offset += block.text.size();
    ++normalized_offset; // logical separator between blocks

    section.text_length = normalized_offset;
    section.blocks.push_back(std::move(block));
}

} // namespace

BookFormat TxtParser::format() const {
    return BookFormat::Txt;
}

ParseResult TxtParser::parse(
    std::string_view bytes,
    const ParserSourceInfo& source
) const {
    ParseResult result;
    result.document.book_id = source.book_id;

    if (bytes.empty()) {
        result.status = ParserStatus::EmptyDocument;
        return result;
    }

    if (hasUtf16Bom(bytes)) {
        result.status = ParserStatus::UnsupportedEncoding;
        return result;
    }

    auto utf8 = stripUtf8Bom(bytes);

    if (!validUtf8(utf8)) {
        result.status = ParserStatus::InvalidUtf8;
        return result;
    }

    auto normalized = normalizeLineEndings(utf8);

    result.document.metadata.title = filenameStem(source.source_filename);
    result.document.metadata.author_display = "Unknown author";
    result.document.metadata.toc_available = false;

    DocumentSection section;
    section.id = "txt:body";

    std::string paragraph;
    std::uint64_t normalized_offset = 0;

    for (const auto& raw_line : splitLines(normalized)) {
        const auto line = trimAsciiWhitespace(raw_line);

        if (line.empty()) {
            flushParagraph(paragraph, section, normalized_offset);
            continue;
        }

        if (!paragraph.empty()) {
            paragraph.push_back(' ');
        }
        paragraph += line;
    }

    flushParagraph(paragraph, section, normalized_offset);

    if (section.blocks.empty()) {
        result.status = ParserStatus::EmptyDocument;
        return result;
    }

    result.document.total_text_length = section.text_length;
    result.document.sections.push_back(std::move(section));
    result.status = ParserStatus::Ok;
    return result;
}

} // namespace enku
