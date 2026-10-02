#include "enku/reader/fb2_parser.hpp"

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
    std::string plain;
    bool in_tag = false;

    for (const char ch : markup) {
        if (!in_tag) {
            if (ch == '<') {
                in_tag = true;
            } else {
                plain.push_back(ch);
            }
        } else if (ch == '>') {
            in_tag = false;
        }
    }

    const auto decoded =
        decodeEntities(plain);

    std::string normalized;
    bool previous_space = false;

    for (const char ch : decoded) {
        const bool space =
            std::isspace(
                static_cast<unsigned char>(ch)
            );

        if (space) {
            if (!previous_space &&
                !normalized.empty()) {
                normalized.push_back(' ');
            }
        } else {
            normalized.push_back(ch);
        }

        previous_space = space;
    }

    return trim(normalized);
}

std::optional<std::string> tagText(
    std::string_view xml,
    std::string_view name,
    std::size_t start = 0
) {
    const auto lower_xml = lower(xml);
    const std::string open =
        "<" + lower(name);
    const std::string close =
        "</" + lower(name) + ">";

    auto pos = lower_xml.find(open, start);

    while (pos != std::string::npos) {
        const auto boundary =
            pos + open.size();

        if (boundary >= lower_xml.size() ||
            lower_xml[boundary] == '>' ||
            std::isspace(
                static_cast<unsigned char>(
                    lower_xml[boundary]
                )
            )) {
            const auto gt =
                lower_xml.find('>', boundary);

            if (gt == std::string::npos) {
                return std::nullopt;
            }

            const auto end =
                lower_xml.find(
                    close,
                    gt + 1U
                );

            if (end == std::string::npos) {
                return std::nullopt;
            }

            return stripTags(
                xml.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );
        }

        pos = lower_xml.find(
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
    const auto lower_xml = lower(xml);
    const std::string open =
        "<" + lower(name);
    const std::string close =
        "</" + lower(name) + ">";

    auto pos = lower_xml.find(open, start);

    while (pos != std::string::npos) {
        const auto boundary =
            pos + open.size();

        if (boundary >= lower_xml.size() ||
            lower_xml[boundary] == '>' ||
            std::isspace(
                static_cast<unsigned char>(
                    lower_xml[boundary]
                )
            )) {
            const auto gt =
                lower_xml.find('>', boundary);
            const auto end =
                gt == std::string::npos
                    ? std::string::npos
                    : lower_xml.find(
                          close,
                          gt + 1U
                      );

            if (gt == std::string::npos ||
                end == std::string::npos) {
                return std::nullopt;
            }

            return xml.substr(
                gt + 1U,
                end - gt - 1U
            );
        }

        pos =
            lower_xml.find(open, boundary);
    }

    return std::nullopt;
}

std::vector<std::string> allTagTexts(
    std::string_view xml,
    std::string_view name
) {
    std::vector<std::string> out;
    const auto lower_xml = lower(xml);
    const std::string open =
        "<" + lower(name);
    const std::string close =
        "</" + lower(name) + ">";

    std::size_t cursor = 0;

    while (true) {
        auto pos =
            lower_xml.find(open, cursor);

        if (pos == std::string::npos) {
            break;
        }

        const auto boundary =
            pos + open.size();

        if (boundary < lower_xml.size() &&
            lower_xml[boundary] != '>' &&
            !std::isspace(
                static_cast<unsigned char>(
                    lower_xml[boundary]
                )
            )) {
            cursor = boundary;
            continue;
        }

        const auto gt =
            lower_xml.find('>', boundary);
        const auto end =
            gt == std::string::npos
                ? std::string::npos
                : lower_xml.find(
                      close,
                      gt + 1U
                  );

        if (gt == std::string::npos ||
            end == std::string::npos) {
            break;
        }

        const auto text =
            stripTags(
                xml.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );

        if (!text.empty()) {
            out.push_back(text);
        }

        cursor = end + close.size();
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
    const auto lower_info =
        lower(title_info);
    std::size_t cursor = 0;

    while (true) {
        const auto begin =
            lower_info.find(
                "<author",
                cursor
            );

        if (begin == std::string::npos) {
            break;
        }

        const auto gt =
            lower_info.find('>', begin);
        const auto end =
            gt == std::string::npos
                ? std::string::npos
                : lower_info.find(
                      "</author>",
                      gt + 1U
                  );

        if (gt == std::string::npos ||
            end == std::string::npos) {
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

struct BodyBlock {
    TextBlockType type;
    std::string text;
};

std::vector<BodyBlock> parseSectionBlocks(
    std::string_view section
) {
    std::vector<BodyBlock> blocks;
    const auto lower_section =
        lower(section);
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
        std::optional<std::size_t> best;
        const Spec* spec = nullptr;

        for (const auto& candidate :
             kSpecs) {
            const std::string needle =
                "<" +
                std::string(candidate.name);

            const auto pos =
                lower_section.find(
                    needle,
                    cursor
                );

            if (pos == std::string::npos) {
                continue;
            }

            const auto boundary =
                pos + needle.size();

            if (boundary < lower_section.size() &&
                lower_section[boundary] != '>' &&
                !std::isspace(
                    static_cast<unsigned char>(
                        lower_section[boundary]
                    )
                )) {
                continue;
            }

            if (!best.has_value() ||
                pos < *best) {
                best = pos;
                spec = &candidate;
            }
        }

        if (!best.has_value() ||
            spec == nullptr) {
            break;
        }

        const auto gt =
            lower_section.find(
                '>',
                *best
            );

        const std::string close =
            "</" +
            std::string(spec->name) +
            ">";

        const auto end =
            gt == std::string::npos
                ? std::string::npos
                : lower_section.find(
                      close,
                      gt + 1U
                  );

        if (gt == std::string::npos ||
            end == std::string::npos) {
            break;
        }

        const auto text =
            stripTags(
                section.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            );

        if (!text.empty()) {
            blocks.push_back(
                BodyBlock{
                    spec->type,
                    text,
                }
            );
        }

        cursor = end + close.size();
    }

    return blocks;
}

} // namespace

BookFormat Fb2Parser::format() const {
    return BookFormat::Fb2;
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

    const auto lowered = lower(bytes);
    if (lowered.find("<fictionbook") ==
        std::string::npos) {
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

    const auto lower_body =
        lower(*body);

    std::size_t cursor = 0;
    std::uint64_t global_offset = 0;
    std::uint32_t section_number = 0;

    while (true) {
        const auto begin =
            lower_body.find(
                "<section",
                cursor
            );

        if (begin == std::string::npos) {
            break;
        }

        const auto gt =
            lower_body.find('>', begin);
        const auto end =
            gt == std::string::npos
                ? std::string::npos
                : lower_body.find(
                      "</section>",
                      gt + 1U
                  );

        if (gt == std::string::npos ||
            end == std::string::npos) {
            break;
        }

        const auto section_xml =
            body->substr(
                gt + 1U,
                end - gt - 1U
            );

        std::string section_content(
            section_xml
        );

        std::optional<std::string> section_title;
        const auto lower_section =
            lower(section_xml);
        const auto title_begin =
            lower_section.find("<title");

        if (title_begin != std::string::npos) {
            const auto title_open_end =
                lower_section.find(
                    '>',
                    title_begin
                );
            const auto title_end =
                title_open_end ==
                        std::string::npos
                    ? std::string::npos
                    : lower_section.find(
                          "</title>",
                          title_open_end + 1U
                      );

            if (title_open_end !=
                    std::string::npos &&
                title_end !=
                    std::string::npos) {
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

            for (const auto& parsed_block :
                 parsed) {
                TextBlock block;
                block.type = parsed_block.type;
                block.text = parsed_block.text;
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

            for (const auto& parsed_block :
                 parsed) {
                TextBlock block;
                block.type = parsed_block.type;
                block.text = parsed_block.text;
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

} // namespace enku
