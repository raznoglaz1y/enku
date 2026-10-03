#include "enku/reader/fb2_parser.hpp"
#include "enku/reader/parser_memory_budget.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace enku {
namespace {

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(
        out.begin(),
        out.end(),
        out.begin(),
        [](unsigned char ch) {
            return static_cast<char>(
                std::tolower(ch)
            );
        }
    );
    return out;
}

std::size_t asciiIFind(
    std::string_view haystack,
    std::string_view needle,
    std::size_t start = 0
) {
    if (needle.empty()) {
        return std::min(
            start,
            haystack.size()
        );
    }

    if (start > haystack.size() ||
        needle.size() >
            haystack.size() - start) {
        return std::string_view::npos;
    }

    const auto matches =
        [](char lhs, char rhs) {
            return std::tolower(
                       static_cast<unsigned char>(
                           lhs
                       )
                   ) ==
                std::tolower(
                       static_cast<unsigned char>(
                           rhs
                       )
                   );
        };

    const auto last =
        haystack.size() - needle.size();

    for (std::size_t pos = start;
         pos <= last;
         ++pos) {
        std::size_t i = 0;

        while (i < needle.size() &&
               matches(
                   haystack[pos + i],
                   needle[i]
               )) {
            ++i;
        }

        if (i == needle.size()) {
            return pos;
        }
    }

    return std::string_view::npos;
}

std::string trim(std::string_view value) {
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(
               static_cast<unsigned char>(
                   value[first]
               )
           )) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(
               static_cast<unsigned char>(
                   value[last - 1]
               )
           )) {
        --last;
    }

    return std::string(
        value.substr(first, last - first)
    );
}

bool appendUtf8(
    std::uint32_t code_point,
    std::string& out
) {
    if (code_point > 0x10FFFFU ||
        (code_point >= 0xD800U &&
         code_point <= 0xDFFFU)) {
        return false;
    }

    if (code_point <= 0x7FU) {
        out.push_back(
            static_cast<char>(code_point)
        );
    } else if (code_point <= 0x7FFU) {
        out.push_back(
            static_cast<char>(
                0xC0U |
                (code_point >> 6U)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                (code_point & 0x3FU)
            )
        );
    } else if (code_point <= 0xFFFFU) {
        out.push_back(
            static_cast<char>(
                0xE0U |
                (code_point >> 12U)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                ((code_point >> 6U) &
                 0x3FU)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                (code_point & 0x3FU)
            )
        );
    } else {
        out.push_back(
            static_cast<char>(
                0xF0U |
                (code_point >> 18U)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                ((code_point >> 12U) &
                 0x3FU)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                ((code_point >> 6U) &
                 0x3FU)
            )
        );
        out.push_back(
            static_cast<char>(
                0x80U |
                (code_point & 0x3FU)
            )
        );
    }

    return true;
}

std::optional<std::uint32_t> numericEntity(
    std::string_view entity
) {
    if (entity.size() < 2U ||
        entity.front() != '#') {
        return std::nullopt;
    }

    int base = 10;
    std::size_t cursor = 1;

    if (cursor < entity.size() &&
        (entity[cursor] == 'x' ||
         entity[cursor] == 'X')) {
        base = 16;
        ++cursor;
    }

    if (cursor >= entity.size()) {
        return std::nullopt;
    }

    std::uint32_t value = 0;

    for (; cursor < entity.size(); ++cursor) {
        const unsigned char ch =
            static_cast<unsigned char>(
                entity[cursor]
            );

        int digit = -1;

        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (
            base == 16 &&
            ch >= 'a' && ch <= 'f'
        ) {
            digit = 10 + ch - 'a';
        } else if (
            base == 16 &&
            ch >= 'A' && ch <= 'F'
        ) {
            digit = 10 + ch - 'A';
        } else {
            return std::nullopt;
        }

        if (digit >= base ||
            value >
                (0x10FFFFU -
                 static_cast<std::uint32_t>(
                     digit
                 )) /
                    static_cast<std::uint32_t>(
                        base
                    )) {
            return std::nullopt;
        }

        value =
            value *
                static_cast<std::uint32_t>(
                    base
                ) +
            static_cast<std::uint32_t>(
                digit
            );
    }

    return value;
}

std::string decodeEntities(
    std::string_view value
) {
    std::string out;
    out.reserve(value.size());

    for (std::size_t i = 0;
         i < value.size();) {
        if (value[i] != '&') {
            out.push_back(value[i++]);
            continue;
        }

        const auto semi =
            value.find(';', i + 1U);

        if (semi == std::string_view::npos) {
            out.push_back(value[i++]);
            continue;
        }

        const auto entity =
            value.substr(
                i + 1U,
                semi - i - 1U
            );

        if (entity == "amp") {
            out.push_back('&');
        } else if (entity == "lt") {
            out.push_back('<');
        } else if (entity == "gt") {
            out.push_back('>');
        } else if (entity == "quot") {
            out.push_back('"');
        } else if (entity == "apos") {
            out.push_back('\'');
        } else if (const auto numeric =
                       numericEntity(entity);
                   numeric.has_value() &&
                   appendUtf8(*numeric, out)) {
            // Numeric XML entity decoded.
        } else {
            out.append(
                value.substr(
                    i,
                    semi - i + 1U
                )
            );
        }

        i = semi + 1U;
    }

    return out;
}

std::string stripTags(
    std::string_view markup
) {
    std::string out;
    out.reserve(markup.size());

    bool in_tag = false;
    bool previous_space = false;
    std::size_t i = 0;

    const auto appendSpace =
        [&]() {
            if (!out.empty() &&
                !previous_space) {
                out.push_back(' ');
                previous_space = true;
            }
        };

    const auto appendByte =
        [&](char ch) {
            if (std::isspace(
                    static_cast<unsigned char>(
                        ch
                    )
                )) {
                appendSpace();
            } else {
                out.push_back(ch);
                previous_space = false;
            }
        };

    while (i < markup.size()) {
        const char ch = markup[i];

        if (in_tag) {
            if (ch == '>') {
                in_tag = false;
            }
            ++i;
            continue;
        }

        if (ch == '<') {
            in_tag = true;
            ++i;
            continue;
        }

        if (ch != '&') {
            appendByte(ch);
            ++i;
            continue;
        }

        const auto semi =
            markup.find(';', i + 1U);

        if (semi == std::string_view::npos) {
            appendByte(ch);
            ++i;
            continue;
        }

        const auto entity =
            markup.substr(
                i + 1U,
                semi - i - 1U
            );

        bool decoded = true;

        if (entity == "amp") {
            appendByte('&');
        } else if (entity == "lt") {
            appendByte('<');
        } else if (entity == "gt") {
            appendByte('>');
        } else if (entity == "quot") {
            appendByte('"');
        } else if (entity == "apos") {
            appendByte('\'');
        } else if (const auto numeric =
                       numericEntity(entity);
                   numeric.has_value()) {
            std::string encoded;
            encoded.reserve(4U);

            if (appendUtf8(
                    *numeric,
                    encoded
                )) {
                for (const char byte :
                     encoded) {
                    appendByte(byte);
                }
            } else {
                decoded = false;
            }
        } else {
            decoded = false;
        }

        if (!decoded) {
            for (std::size_t raw = i;
                 raw <= semi;
                 ++raw) {
                appendByte(markup[raw]);
            }
        }

        i = semi + 1U;
    }

    while (!out.empty() &&
           std::isspace(
               static_cast<unsigned char>(
                   out.back()
               )
           )) {
        out.pop_back();
    }

    return out;
}

std::optional<std::string> tagText(
    std::string_view xml,
    std::string_view name,
    std::size_t start = 0
) {
    const std::string open =
        "<" + std::string(name);
    const std::string close =
        "</" + std::string(name) + ">";

    auto pos =
        asciiIFind(
            xml,
            open,
            start
        );

    while (pos != std::string_view::npos) {
        const auto boundary =
            pos + open.size();

        if (boundary >= xml.size() ||
            xml[boundary] == '>' ||
            std::isspace(
                static_cast<unsigned char>(
                    xml[boundary]
                )
            )) {
            const auto gt =
                xml.find('>', boundary);

            if (gt == std::string_view::npos) {
                return std::nullopt;
            }

            const auto end =
                asciiIFind(
                    xml,
                    close,
                    gt + 1U
                );

            if (end == std::string_view::npos) {
                return std::nullopt;
            }

            return stripTags(
                xml.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );
        }

        pos =
            asciiIFind(
                xml,
                open,
                boundary
            );
    }

    return std::nullopt;
}

std::optional<std::string_view> tagSlice(
    std::string_view xml,
    std::string_view name,
    std::size_t start = 0
) {
    const std::string open =
        "<" + std::string(name);
    const std::string close =
        "</" + std::string(name) + ">";

    auto pos =
        asciiIFind(
            xml,
            open,
            start
        );

    while (pos != std::string_view::npos) {
        const auto boundary =
            pos + open.size();

        if (boundary >= xml.size() ||
            xml[boundary] == '>' ||
            std::isspace(
                static_cast<unsigned char>(
                    xml[boundary]
                )
            )) {
            const auto gt =
                xml.find('>', boundary);
            const auto end =
                gt == std::string_view::npos
                    ? std::string_view::npos
                    : asciiIFind(
                          xml,
                          close,
                          gt + 1U
                      );

            if (gt == std::string_view::npos ||
                end == std::string_view::npos) {
                return std::nullopt;
            }

            return xml.substr(
                gt + 1U,
                end - gt - 1U
            );
        }

        pos =
            asciiIFind(
                xml,
                open,
                boundary
            );
    }

    return std::nullopt;
}

std::vector<std::string> allTagTexts(
    std::string_view xml,
    std::string_view name
) {
    std::vector<std::string> out;
    const std::string open =
        "<" + std::string(name);
    const std::string close =
        "</" + std::string(name) + ">";

    std::size_t cursor = 0;

    while (true) {
        auto pos =
            asciiIFind(
                xml,
                open,
                cursor
            );

        if (pos == std::string_view::npos) {
            break;
        }

        const auto boundary =
            pos + open.size();

        if (boundary < xml.size() &&
            xml[boundary] != '>' &&
            !std::isspace(
                static_cast<unsigned char>(
                    xml[boundary]
                )
            )) {
            cursor = boundary;
            continue;
        }

        const auto gt =
            xml.find('>', boundary);

        if (gt == std::string_view::npos) {
            break;
        }

        const auto end =
            asciiIFind(
                xml,
                close,
                gt + 1U
            );

        if (end == std::string_view::npos) {
            break;
        }

        auto value =
            stripTags(
                xml.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );

        if (!value.empty()) {
            out.push_back(
                std::move(value)
            );
        }

        cursor =
            end + close.size();
    }

    return out;
}

bool declaresNonUtf8(
    std::string_view bytes
) {
    const auto head =
        lower(
            bytes.substr(
                0,
                std::min<std::size_t>(
                    bytes.size(),
                    256U
                )
            )
        );

    const auto encoding =
        head.find("encoding=");

    if (encoding == std::string::npos) {
        return false;
    }

    const auto utf8 =
        head.find("utf-8", encoding);
    const auto utf8_compact =
        head.find("utf8", encoding);

    return utf8 == std::string::npos &&
        utf8_compact == std::string::npos;
}

std::string authorName(
    std::string_view author
) {
    std::string name;

    for (const auto* field :
         {"first-name", "middle-name", "last-name"}) {
        const auto part =
            tagText(author, field);

        if (part.has_value() &&
            !part->empty()) {
            if (!name.empty()) {
                name.push_back(' ');
            }
            name += *part;
        }
    }

    if (name.empty()) {
        const auto nickname =
            tagText(author, "nickname");

        if (nickname.has_value()) {
            name = *nickname;
        }
    }

    return name;
}

std::vector<std::string> authors(
    std::string_view title_info
) {
    std::vector<std::string> result;
    std::size_t cursor = 0;

    while (true) {
        const auto begin =
            asciiIFind(
                title_info,
                "<author",
                cursor
            );

        if (begin == std::string_view::npos) {
            break;
        }

        const auto gt =
            title_info.find('>', begin);
        const auto end =
            gt == std::string_view::npos
                ? std::string_view::npos
                : asciiIFind(
                      title_info,
                      "</author>",
                      gt + 1U
                  );

        if (gt == std::string_view::npos ||
            end == std::string_view::npos) {
            break;
        }

        const auto name =
            authorName(
                title_info.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );

        if (!name.empty()) {
            result.push_back(name);
        }

        cursor = end + 9U;
    }

    return result;
}

std::optional<std::size_t> matchingSectionClose(
    std::string_view xml,
    std::size_t open_start
) {
    const auto open_end =
        xml.find('>', open_start);

    if (open_end == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t cursor = open_end + 1U;
    std::uint32_t depth = 1U;

    while (cursor < xml.size()) {
        const auto next_open =
            asciiIFind(
                xml,
                "<section",
                cursor
            );
        const auto next_close =
            asciiIFind(
                xml,
                "</section>",
                cursor
            );

        if (next_close == std::string_view::npos) {
            return std::nullopt;
        }

        if (next_open != std::string_view::npos &&
            next_open < next_close) {
            const auto boundary =
                next_open + 8U;

            if (boundary >= xml.size() ||
                xml[boundary] == '>' ||
                std::isspace(
                    static_cast<unsigned char>(
                        xml[boundary]
                    )
                )) {
                ++depth;
            }

            cursor = boundary;
            continue;
        }

        --depth;

        if (depth == 0U) {
            return next_close;
        }

        cursor = next_close + 10U;
    }

    return std::nullopt;
}

bool asciiIEquals(
    std::string_view value,
    std::string_view expected
) {
    if (value.size() != expected.size()) {
        return false;
    }

    for (std::size_t i = 0;
         i < value.size();
         ++i) {
        if (std::tolower(
                static_cast<unsigned char>(
                    value[i]
                )
            ) !=
            std::tolower(
                static_cast<unsigned char>(
                    expected[i]
                )
            )) {
            return false;
        }
    }

    return true;
}

std::optional<std::pair<
    std::size_t,
    std::size_t
>> nextOpenTag(
    std::string_view markup,
    std::size_t cursor,
    std::string_view name
) {
    while (cursor < markup.size()) {
        const auto open =
            markup.find('<', cursor);

        if (open == std::string_view::npos ||
            open + 1U >= markup.size()) {
            return std::nullopt;
        }

        const auto first =
            markup[open + 1U];

        if (first == '/' ||
            first == '!' ||
            first == '?') {
            cursor = open + 2U;
            continue;
        }

        const auto name_start =
            open + 1U;
        std::size_t name_end =
            name_start;

        while (name_end < markup.size()) {
            const unsigned char ch =
                static_cast<unsigned char>(
                    markup[name_end]
                );

            if (!(std::isalnum(ch) ||
                  ch == ':' ||
                  ch == '-' ||
                  ch == '_')) {
                break;
            }

            ++name_end;
        }

        if (asciiIEquals(
                markup.substr(
                    name_start,
                    name_end - name_start
                ),
                name
            )) {
            const auto gt =
                markup.find('>', name_end);

            if (gt == std::string_view::npos) {
                return std::nullopt;
            }

            return std::pair{
                open,
                gt,
            };
        }

        cursor =
            name_end > open
                ? name_end
                : open + 1U;
    }

    return std::nullopt;
}

std::optional<std::pair<
    std::size_t,
    std::size_t
>> closingTag(
    std::string_view markup,
    std::size_t cursor,
    std::string_view name
) {
    while (cursor < markup.size()) {
        const auto open =
            markup.find('<', cursor);

        if (open == std::string_view::npos ||
            open + 2U >= markup.size()) {
            return std::nullopt;
        }

        if (markup[open + 1U] != '/') {
            cursor = open + 1U;
            continue;
        }

        const auto name_start =
            open + 2U;
        std::size_t name_end =
            name_start;

        while (name_end < markup.size()) {
            const unsigned char ch =
                static_cast<unsigned char>(
                    markup[name_end]
                );

            if (!(std::isalnum(ch) ||
                  ch == ':' ||
                  ch == '-' ||
                  ch == '_')) {
                break;
            }

            ++name_end;
        }

        if (asciiIEquals(
                markup.substr(
                    name_start,
                    name_end - name_start
                ),
                name
            )) {
            const auto gt =
                markup.find('>', name_end);

            if (gt == std::string_view::npos) {
                return std::nullopt;
            }

            return std::pair{
                open,
                gt,
            };
        }

        cursor =
            name_end > open
                ? name_end
                : open + 1U;
    }

    return std::nullopt;
}

struct BodyBlock {
    TextBlockType type;
    std::string text;
};

std::vector<BodyBlock> parseSectionBlocks(
    std::string_view section
) {
    std::vector<BodyBlock> blocks;
    std::size_t cursor = 0;

    struct Spec {
        const char* name;
        TextBlockType type;
    };

    static constexpr Spec kSpecs[] = {
        {"subtitle", TextBlockType::Heading},
        {"cite", TextBlockType::Quote},
        {"p", TextBlockType::Paragraph},
    };

    while (cursor < section.size()) {
        std::optional<std::pair<
            std::size_t,
            std::size_t
        >> best_open;
        const Spec* best_spec = nullptr;

        for (const auto& spec : kSpecs) {
            const auto candidate =
                nextOpenTag(
                    section,
                    cursor,
                    spec.name
                );

            if (!candidate.has_value()) {
                continue;
            }

            if (!best_open.has_value() ||
                candidate->first <
                    best_open->first) {
                best_open = candidate;
                best_spec = &spec;
            }
        }

        if (!best_open.has_value() ||
            best_spec == nullptr) {
            break;
        }

        const auto close =
            closingTag(
                section,
                best_open->second + 1U,
                best_spec->name
            );

        if (!close.has_value()) {
            cursor =
                best_open->second + 1U;
            continue;
        }

        auto text =
            stripTags(
                section.substr(
                    best_open->second + 1U,
                    close->first -
                        best_open->second - 1U
                )
            );

        if (!text.empty()) {
            blocks.push_back(
                BodyBlock{
                    best_spec->type,
                    std::move(text),
                }
            );
        }

        cursor =
            close->second + 1U;
    }

    return blocks;
}

} // namespace

BookFormat Fb2Parser::format() const {
    return BookFormat::Fb2;
}

ParseResult Fb2Parser::parseMetadata(
    std::string_view bytes,
    const ParserSourceInfo& source
) const {
    ParseResult result;
    result.document.book_id = source.book_id;

    if (bytes.empty()) {
        result.status = ParserStatus::EmptyDocument;
        return result;
    }

    if (declaresNonUtf8(bytes)) {
        result.status =
            ParserStatus::UnsupportedEncoding;
        return result;
    }

    if (asciiIFind(
            bytes,
            "<fictionbook"
        ) == std::string_view::npos) {
        result.status = ParserStatus::InvalidSource;
        return result;
    }

    const auto description =
        tagSlice(bytes, "description");
    const auto title_info =
        description.has_value()
            ? tagSlice(
                  *description,
                  "title-info"
              )
            : std::nullopt;

    result.document.metadata.title =
        title_info.has_value()
            ? tagText(
                  *title_info,
                  "book-title"
              ).value_or(
                  source.source_filename
              )
            : source.source_filename;

    if (title_info.has_value()) {
        result.document.metadata.authors =
            authors(*title_info);
        result.document.metadata.language =
            tagText(*title_info, "lang");
        result.document.metadata.description =
            tagText(
                *title_info,
                "annotation"
            );
    }

    if (result.document.metadata.authors.empty()) {
        result.document.metadata.author_display =
            "Unknown author";
    } else {
        result.document.metadata.author_display =
            result.document.metadata.authors.front();

        for (std::size_t i = 1;
             i < result.document.metadata.authors.size();
             ++i) {
            result.document.metadata.author_display +=
                ", " +
                result.document.metadata.authors[i];
        }
    }

    const auto publish_info =
        description.has_value()
            ? tagSlice(
                  *description,
                  "publish-info"
              )
            : std::nullopt;

    if (publish_info.has_value()) {
        result.document.metadata.publisher =
            tagText(
                *publish_info,
                "publisher"
            );
        result.document.metadata.published_date =
            tagText(
                *publish_info,
                "year"
            );
        result.document.metadata.identifier =
            tagText(
                *publish_info,
                "isbn"
            );
    }

    const auto body_position =
        asciiIFind(
            bytes,
            "<body"
        );

    if (body_position ==
        std::string_view::npos) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    result.document.metadata.toc_available =
        asciiIFind(
            bytes,
            "<title",
            body_position
        ) != std::string_view::npos;

    result.status = ParserStatus::Ok;
    return result;
}

ParseResult Fb2Parser::parseMetadata(
    const Fb2RangeSource& source_bytes,
    const ParserSourceInfo& source
) const {
    constexpr std::size_t kChunkBytes =
        ParserMemoryBudget::kRangeChunkBytes;
    constexpr std::size_t kMaxMetadataBytes =
        ParserMemoryBudget::kFb2MetadataBytes;

    if (source_bytes.size() == 0U) {
        ParseResult result;
        result.document.book_id =
            source.book_id;
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    std::string buffer;
    buffer.reserve(
        std::min<std::uint64_t>(
            source_bytes.size(),
            kMaxMetadataBytes
        )
    );

    std::uint64_t offset = 0;

    while (offset < source_bytes.size() &&
           buffer.size() <
               kMaxMetadataBytes) {
        const auto remaining =
            source_bytes.size() - offset;

        const auto allowed =
            kMaxMetadataBytes -
            buffer.size();

        const auto requested =
            static_cast<std::size_t>(
                std::min<std::uint64_t>(
                    remaining,
                    std::min<std::size_t>(
                        kChunkBytes,
                        allowed
                    )
                )
            );

        if (requested == 0U) {
            break;
        }

        std::string chunk;

        if (!source_bytes.readRange(
                offset,
                requested,
                chunk
            ) ||
            chunk.size() != requested) {
            ParseResult result;
            result.document.book_id =
                source.book_id;
            result.status =
                ParserStatus::InvalidSource;
            return result;
        }

        buffer += chunk;
        offset +=
            static_cast<std::uint64_t>(
                requested
            );

        const auto description_end =
            asciiIFind(
                buffer,
                "</description>"
            );
        const auto body_start =
            asciiIFind(
                buffer,
                "<body"
            );

        if (description_end !=
                std::string_view::npos &&
            body_start !=
                std::string_view::npos &&
            body_start >
                description_end) {
            return parseMetadata(
                buffer,
                source
            );
        }
    }

    return parseMetadata(
        buffer,
        source
    );
}

ParseResult Fb2Parser::parse(
    std::string_view bytes,
    const ParserSourceInfo& source
) const {
    ParseResult result;
    result.document.book_id = source.book_id;

    if (bytes.empty()) {
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    if (declaresNonUtf8(bytes)) {
        result.status =
            ParserStatus::UnsupportedEncoding;
        return result;
    }

    if (asciiIFind(
            bytes,
            "<fictionbook"
        ) == std::string_view::npos) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    const auto description =
        tagSlice(bytes, "description");
    const auto title_info =
        description.has_value()
            ? tagSlice(
                  *description,
                  "title-info"
              )
            : std::nullopt;

    result.document.metadata.title =
        title_info.has_value()
            ? tagText(
                  *title_info,
                  "book-title"
              ).value_or(
                  source.source_filename
              )
            : source.source_filename;

    if (title_info.has_value()) {
        result.document.metadata.authors =
            authors(*title_info);

        result.document.metadata.language =
            tagText(*title_info, "lang");
        result.document.metadata.description =
            tagText(*title_info, "annotation");
    }

    if (result.document.metadata.authors.empty()) {
        result.document.metadata.author_display =
            "Unknown author";
    } else {
        result.document.metadata.author_display =
            result.document.metadata.authors.front();

        for (std::size_t i = 1;
             i < result.document.metadata.authors.size();
             ++i) {
            result.document.metadata.author_display +=
                ", " +
                result.document.metadata.authors[i];
        }
    }

    const auto publish_info =
        description.has_value()
            ? tagSlice(
                  *description,
                  "publish-info"
              )
            : std::nullopt;

    if (publish_info.has_value()) {
        result.document.metadata.publisher =
            tagText(
                *publish_info,
                "publisher"
            );
        result.document.metadata.published_date =
            tagText(
                *publish_info,
                "year"
            );
        result.document.metadata.identifier =
            tagText(
                *publish_info,
                "isbn"
            );
    }

    const auto body =
        tagSlice(bytes, "body");

    if (!body.has_value()) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    std::size_t cursor = 0;
    std::uint64_t global_offset = 0;
    std::uint32_t section_number = 0;

    while (true) {
        const auto begin =
            asciiIFind(
                *body,
                "<section",
                cursor
            );

        if (begin == std::string_view::npos) {
            break;
        }

        const auto gt =
            body->find('>', begin);

        if (gt == std::string_view::npos) {
            break;
        }

        const auto matched_end =
            matchingSectionClose(
                *body,
                begin
            );

        if (!matched_end.has_value()) {
            break;
        }

        const auto end =
            *matched_end;

        const auto section_xml =
            body->substr(
                gt + 1U,
                end - gt - 1U
            );

        const bool has_nested_sections =
            asciiIFind(
                section_xml,
                "<section"
            ) != std::string_view::npos;

        if (has_nested_sections) {
            // Parent sections are structural containers. Advance only past
            // the opening tag so nested leaf sections are emitted as the
            // actual reading units.
            cursor = gt + 1U;
            continue;
        }

        std::string section_content(
            section_xml
        );

        std::optional<std::string> section_title;
        const auto title_begin =
            asciiIFind(
                section_xml,
                "<title"
            );

        if (title_begin != std::string_view::npos) {
            const auto title_open_end =
                section_xml.find(
                    '>',
                    title_begin
                );
            const auto title_end =
                title_open_end ==
                        std::string_view::npos
                    ? std::string_view::npos
                    : asciiIFind(
                          section_xml,
                          "</title>",
                          title_open_end + 1U
                      );

            if (title_open_end !=
                    std::string_view::npos &&
                title_end !=
                    std::string_view::npos) {
                const auto title_text =
                    stripTags(
                        section_xml.substr(
                            title_open_end + 1U,
                            title_end -
                                title_open_end - 1U
                        )
                    );

                if (!title_text.empty()) {
                    section_title =
                        title_text;
                }

                section_content.erase(
                    title_begin,
                    title_end +
                        std::string("</title>").
                            size() -
                        title_begin
                );
            }
        }

        const auto parsed =
            parseSectionBlocks(
                section_content
            );

        if (!parsed.empty()) {
            DocumentSection section;
            section.id =
                "fb2:section:" +
                std::to_string(
                    ++section_number
                );
            section.title =
                section_title;

            std::uint64_t section_offset = 0;

            for (auto& parsed_block :
                 parsed) {
                TextBlock block;
                block.type = parsed_block.type;
                block.text =
                    std::move(
                        parsed_block.text
                    );
                block.text_offset =
                    global_offset +
                    section_offset;

                section_offset +=
                    static_cast<std::uint64_t>(
                        block.text.size()
                    ) + 1U;

                section.blocks.push_back(
                    std::move(block)
                );
            }

            section.text_length =
                section_offset;
            global_offset +=
                section_offset;

            result.document.sections.push_back(
                std::move(section)
            );
        }

        cursor = end + 10U;
    }

    if (result.document.sections.empty()) {
        const auto parsed =
            parseSectionBlocks(*body);

        if (!parsed.empty()) {
            DocumentSection section;
            section.id = "fb2:body";

            std::uint64_t offset = 0;

            for (auto& parsed_block :
                 parsed) {
                TextBlock block;
                block.type = parsed_block.type;
                block.text =
                    std::move(
                        parsed_block.text
                    );
                block.text_offset = offset;
                offset +=
                    static_cast<std::uint64_t>(
                        block.text.size()
                    ) + 1U;
                section.blocks.push_back(
                    std::move(block)
                );
            }

            section.text_length = offset;
            global_offset = offset;
            result.document.sections.push_back(
                std::move(section)
            );
        }
    }

    if (result.document.sections.empty()) {
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    result.document.metadata.toc_available =
        std::any_of(
            result.document.sections.begin(),
            result.document.sections.end(),
            [](const DocumentSection& section) {
                return section.title.has_value() &&
                    !section.title->empty();
            }
        );

    result.document.total_text_length =
        global_offset;
    result.status = ParserStatus::Ok;
    return result;
}


ParseResult Fb2Parser::parse(
    const Fb2RangeSource& source_bytes,
    const ParserSourceInfo& source
) const {
    constexpr std::size_t kChunkBytes =
        ParserMemoryBudget::kRangeChunkBytes;
    constexpr std::size_t kMaxLeafSectionBytes =
        ParserMemoryBudget::kTextResourceBytes;

    auto result =
        parseMetadata(
            source_bytes,
            source
        );

    if (!result.ok()) {
        return result;
    }

    result.document.sections.clear();
    result.document.total_text_length = 0;

    struct SectionFrame {
        bool root{false};
        bool has_nested{false};
        bool capture{true};
        std::string content;
    };

    std::vector<SectionFrame> frames;
    std::string tag;
    bool in_tag = false;
    bool in_body = false;
    bool finished_body = false;
    std::uint64_t global_offset = 0;
    std::uint32_t section_number = 0;

    const auto isOpeningTag =
        [](std::string_view value,
           std::string_view name) {
            if (value.size() < name.size() ||
                value.substr(
                    0,
                    name.size()
                ) != name) {
                return false;
            }

            return value.size() == name.size() ||
                std::isspace(
                    static_cast<unsigned char>(
                        value[name.size()]
                    )
                ) ||
                value[name.size()] == '/';
        };

    const auto appendLeaf =
        [&](std::string section_xml,
            bool root) {
            std::optional<std::string>
                section_title;

            const auto title_open =
                nextOpenTag(
                    section_xml,
                    0U,
                    "title"
                );

            if (title_open.has_value()) {
                const auto title_close =
                    closingTag(
                        section_xml,
                        title_open->second + 1U,
                        "title"
                    );

                if (title_close.has_value()) {
                    auto title_text =
                        stripTags(
                            std::string_view(
                                section_xml
                            ).substr(
                                title_open->second + 1U,
                                title_close->first -
                                    title_open->second -
                                    1U
                            )
                        );

                    if (!title_text.empty()) {
                        section_title =
                            std::move(
                                title_text
                            );
                    }

                    section_xml.erase(
                        title_open->first,
                        title_close->second + 1U -
                            title_open->first
                    );
                }
            }

            auto parsed =
                parseSectionBlocks(
                    section_xml
                );

            if (parsed.empty()) {
                return;
            }

            DocumentSection section;
            section.id =
                root
                    ? "fb2:body"
                    : "fb2:section:" +
                        std::to_string(
                            ++section_number
                        );
            section.title =
                section_title;

            std::uint64_t section_offset = 0;

            for (auto& parsed_block :
                 parsed) {
                TextBlock block;
                block.type =
                    parsed_block.type;
                block.text =
                    std::move(
                        parsed_block.text
                    );
                block.text_offset =
                    global_offset +
                    section_offset;

                section_offset +=
                    static_cast<std::uint64_t>(
                        block.text.size()
                    ) + 1U;

                section.blocks.push_back(
                    std::move(block)
                );
            }

            section.text_length =
                section_offset;
            global_offset +=
                section_offset;

            result.document.sections.push_back(
                std::move(section)
            );
        };

    bool section_limit_exceeded = false;

    const auto appendToCurrent =
        [&](std::string_view bytes) {
            if (frames.empty() ||
                !frames.back().capture) {
                return;
            }

            auto& content =
                frames.back().content;

            if (bytes.size() >
                    kMaxLeafSectionBytes ||
                content.size() >
                    kMaxLeafSectionBytes -
                        bytes.size()) {
                section_limit_exceeded = true;
                return;
            }

            content.append(bytes);
        };

    std::uint64_t offset = 0;

    while (offset < source_bytes.size() &&
           !finished_body) {
        const auto remaining =
            source_bytes.size() -
            offset;

        const auto requested =
            static_cast<std::size_t>(
                std::min<std::uint64_t>(
                    remaining,
                    kChunkBytes
                )
            );

        std::string chunk;

        if (!source_bytes.readRange(
                offset,
                requested,
                chunk
            ) ||
            chunk.size() != requested) {
            result.status =
                ParserStatus::InvalidSource;
            result.document.sections.clear();
            return result;
        }

        offset +=
            static_cast<std::uint64_t>(
                requested
            );

        for (const char ch : chunk) {
            if (!in_tag) {
                if (ch == '<') {
                    in_tag = true;
                    tag.clear();
                    continue;
                }

                if (in_body) {
                    appendToCurrent(
                        std::string_view(
                            &ch,
                            1U
                        )
                    );
                }

                continue;
            }

            if (ch != '>') {
                tag.push_back(ch);
                continue;
            }

            in_tag = false;

            const auto normalized =
                lower(trim(tag));

            if (isOpeningTag(
                    normalized,
                    "body"
                )) {
                if (!in_body) {
                    in_body = true;
                    frames.push_back(
                        SectionFrame{
                            true,
                            false,
                            true,
                            {},
                        }
                    );
                }
                continue;
            }

            if (normalized == "/body") {
                if (in_body) {
                    if (!frames.empty()) {
                        auto root =
                            std::move(
                                frames.front()
                            );

                        if (root.root &&
                            root.capture &&
                            !root.has_nested) {
                            appendLeaf(
                                std::move(
                                    root.content
                                ),
                                true
                            );
                        }
                    }

                    frames.clear();
                    in_body = false;
                    finished_body = true;
                }
                continue;
            }

            if (!in_body) {
                continue;
            }

            if (isOpeningTag(
                    normalized,
                    "section"
                ) &&
                (normalized.empty() ||
                 normalized.front() != '/')) {
                if (!frames.empty()) {
                    frames.back().has_nested =
                        true;
                    frames.back().capture =
                        false;
                    frames.back().content.clear();
                }

                frames.push_back(
                    SectionFrame{
                        false,
                        false,
                        true,
                        {},
                    }
                );
                continue;
            }

            if (normalized == "/section") {
                if (frames.size() <= 1U) {
                    result.status =
                        ParserStatus::InvalidSource;
                    result.document.sections.clear();
                    return result;
                }

                auto frame =
                    std::move(
                        frames.back()
                    );
                frames.pop_back();

                if (frame.capture &&
                    !frame.has_nested) {
                    appendLeaf(
                        std::move(
                            frame.content
                        ),
                        false
                    );
                }
                continue;
            }

            if (!frames.empty() &&
                frames.back().capture) {
                appendToCurrent("<");
                appendToCurrent(tag);
                appendToCurrent(">");
            }

            if (section_limit_exceeded) {
                result.status =
                    ParserStatus::InvalidSource;
                result.document.sections.clear();
                return result;
            }
        }
    }

    if (!finished_body ||
        in_tag ||
        !frames.empty()) {
        result.status =
            ParserStatus::InvalidSource;
        result.document.sections.clear();
        return result;
    }

    if (result.document.sections.empty()) {
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    result.document.metadata.toc_available =
        std::any_of(
            result.document.sections.begin(),
            result.document.sections.end(),
            [](const DocumentSection& section) {
                return section.title.has_value() &&
                    !section.title->empty();
            }
        );

    result.document.total_text_length =
        global_offset;
    result.status = ParserStatus::Ok;
    return result;
}

} // namespace enku
