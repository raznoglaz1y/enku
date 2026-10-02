#include "enku/reader/epub_parser.hpp"
#include "enku/reader/zip_archive.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace enku {
namespace {

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

std::string xmlDecode(std::string_view value) {
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
            // Numeric XML/HTML entity decoded.
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

std::optional<std::string> attribute(
    std::string_view tag,
    std::string_view name
) {
    const std::string needle =
        std::string(name) + "=";

    auto pos = tag.find(needle);

    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    pos += needle.size();

    while (pos < tag.size() &&
           std::isspace(
               static_cast<unsigned char>(
                   tag[pos]
               )
           )) {
        ++pos;
    }

    if (pos >= tag.size() ||
        (tag[pos] != '"' &&
         tag[pos] != '\'')) {
        return std::nullopt;
    }

    const char quote = tag[pos++];
    const auto end = tag.find(quote, pos);

    if (end == std::string_view::npos) {
        return std::nullopt;
    }

    return xmlDecode(
        tag.substr(pos, end - pos)
    );
}

std::optional<std::string> firstTagText(
    std::string_view xml,
    std::string_view local_name
) {
    const std::string lower_xml = lower(xml);
    const std::string needle =
        "<" + lower(local_name);

    std::size_t search = 0;

    while (true) {
        const auto open =
            lower_xml.find(needle, search);

        if (open == std::string::npos) {
            return std::nullopt;
        }

        const auto name_end =
            open + needle.size();

        if (name_end < lower_xml.size() &&
            lower_xml[name_end] != '>' &&
            lower_xml[name_end] != ' ' &&
            lower_xml[name_end] != '\t' &&
            lower_xml[name_end] != '\r' &&
            lower_xml[name_end] != '\n') {
            search = name_end;
            continue;
        }

        const auto gt =
            lower_xml.find('>', name_end);

        if (gt == std::string::npos) {
            return std::nullopt;
        }

        const std::string close =
            "</" + lower(local_name) + ">";

        const auto end =
            lower_xml.find(close, gt + 1U);

        if (end == std::string::npos) {
            return std::nullopt;
        }

        return trim(
            xmlDecode(
                xml.substr(
                    gt + 1U,
                    end - gt - 1U
                )
            )
        );
    }
}

std::optional<std::string> dcText(
    std::string_view xml,
    std::string_view local_name
) {
    const std::string prefixed =
        "dc:" + std::string(local_name);

    if (auto value =
            firstTagText(xml, prefixed);
        value.has_value()) {
        return value;
    }

    return firstTagText(xml, local_name);
}

std::string directoryOf(
    std::string_view path
) {
    const auto slash =
        path.find_last_of('/');

    return slash == std::string_view::npos
        ? std::string{}
        : std::string(path.substr(0, slash + 1U));
}

std::string normalizePath(
    std::string_view base,
    std::string_view relative
) {
    std::string combined;

    if (!relative.empty() &&
        relative.front() == '/') {
        combined =
            std::string(relative.substr(1));
    } else {
        combined =
            std::string(base) +
            std::string(relative);
    }

    std::vector<std::string> parts;
    std::size_t start = 0;

    while (start <= combined.size()) {
        const auto slash =
            combined.find('/', start);

        const auto part =
            slash == std::string::npos
                ? combined.substr(start)
                : combined.substr(
                      start,
                      slash - start
                  );

        if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else if (!part.empty() &&
                   part != ".") {
            parts.push_back(part);
        }

        if (slash == std::string::npos) {
            break;
        }

        start = slash + 1U;
    }

    std::string out;

    for (std::size_t i = 0;
         i < parts.size();
         ++i) {
        if (i != 0U) {
            out.push_back('/');
        }
        out += parts[i];
    }

    return out;
}

std::string stripInlineTags(
    std::string_view markup
) {
    std::string text;
    text.reserve(markup.size());

    bool in_tag = false;
    std::string tag;

    for (const char ch : markup) {
        if (!in_tag) {
            if (ch == '<') {
                in_tag = true;
                tag.clear();
            } else {
                text.push_back(ch);
            }
        } else if (ch == '>') {
            in_tag = false;

            const auto lowered =
                lower(trim(tag));

            if (lowered == "br" ||
                lowered == "br/" ||
                lowered.rfind(
                    "br ",
                    0
                ) == 0) {
                if (!text.empty() &&
                    !std::isspace(
                        static_cast<unsigned char>(
                            text.back()
                        )
                    )) {
                    text.push_back(' ');
                }
            }
        } else {
            tag.push_back(ch);
        }
    }

    std::string decoded =
        xmlDecode(text);

    std::string normalized;
    normalized.reserve(decoded.size());
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

struct ParsedBlock {
    TextBlockType type{TextBlockType::Paragraph};
    std::string text;
};

std::vector<ParsedBlock> parseXhtmlBlocks(
    std::string_view markup
) {
    const std::string lower_markup =
        lower(markup);

    struct TagSpec {
        const char* name;
        TextBlockType type;
    };

    static constexpr TagSpec kTags[] = {
        {"h1", TextBlockType::Heading},
        {"h2", TextBlockType::Heading},
        {"h3", TextBlockType::Heading},
        {"h4", TextBlockType::Heading},
        {"h5", TextBlockType::Heading},
        {"h6", TextBlockType::Heading},
        {"blockquote", TextBlockType::Quote},
        {"li", TextBlockType::ListItem},
        {"p", TextBlockType::Paragraph},
    };

    std::vector<ParsedBlock> blocks;
    std::size_t cursor = 0;

    while (cursor < markup.size()) {
        std::optional<std::size_t> best_pos;
        const TagSpec* best_spec = nullptr;

        for (const auto& spec : kTags) {
            const std::string needle =
                "<" + std::string(spec.name);

            const auto pos =
                lower_markup.find(
                    needle,
                    cursor
                );

            if (pos == std::string::npos) {
                continue;
            }

            const auto boundary =
                pos + needle.size();

            if (boundary < lower_markup.size() &&
                lower_markup[boundary] != '>' &&
                lower_markup[boundary] != '/' &&
                !std::isspace(
                    static_cast<unsigned char>(
                        lower_markup[boundary]
                    )
                )) {
                continue;
            }

            if (!best_pos.has_value() ||
                pos < *best_pos) {
                best_pos = pos;
                best_spec = &spec;
            }
        }

        if (!best_pos.has_value() ||
            best_spec == nullptr) {
            break;
        }

        const auto open_end =
            lower_markup.find(
                '>',
                *best_pos
            );

        if (open_end == std::string::npos) {
            break;
        }

        const std::string close =
            "</" +
            std::string(best_spec->name) +
            ">";

        const auto close_pos =
            lower_markup.find(
                close,
                open_end + 1U
            );

        if (close_pos == std::string::npos) {
            cursor = open_end + 1U;
            continue;
        }

        const auto inner =
            markup.substr(
                open_end + 1U,
                close_pos - open_end - 1U
            );

        const auto text =
            stripInlineTags(inner);

        if (!text.empty()) {
            blocks.push_back(
                ParsedBlock{
                    best_spec->type,
                    text,
                }
            );
        }

        cursor =
            close_pos +
            close.size();
    }

    if (!blocks.empty()) {
        return blocks;
    }

    const auto fallback =
        stripInlineTags(markup);

    if (!fallback.empty()) {
        blocks.push_back(
            ParsedBlock{
                TextBlockType::Paragraph,
                fallback,
            }
        );
    }

    return blocks;
}

std::vector<std::string> allTagText(
    std::string_view xml,
    std::string_view local_name
) {
    std::vector<std::string> values;
    const std::string lower_xml = lower(xml);
    const std::string needle =
        "<" + lower(local_name);
    const std::string close =
        "</" + lower(local_name) + ">";

    std::size_t search = 0;

    while (true) {
        const auto open =
            lower_xml.find(needle, search);

        if (open == std::string::npos) {
            break;
        }

        const auto name_end =
            open + needle.size();

        if (name_end < lower_xml.size() &&
            lower_xml[name_end] != '>' &&
            !std::isspace(
                static_cast<unsigned char>(
                    lower_xml[name_end]
                )
            )) {
            search = name_end;
            continue;
        }

        const auto gt =
            lower_xml.find('>', name_end);

        if (gt == std::string::npos) {
            break;
        }

        const auto end =
            lower_xml.find(
                close,
                gt + 1U
            );

        if (end == std::string::npos) {
            break;
        }

        const auto value =
            trim(
                xmlDecode(
                    xml.substr(
                        gt + 1U,
                        end - gt - 1U
                    )
                )
            );

        if (!value.empty()) {
            values.push_back(value);
        }

        search = end + close.size();
    }

    return values;
}

std::vector<std::string> dcTexts(
    std::string_view xml,
    std::string_view local_name
) {
    auto values =
        allTagText(
            xml,
            "dc:" + std::string(local_name)
        );

    if (values.empty()) {
        values =
            allTagText(
                xml,
                local_name
            );
    }

    return values;
}

std::optional<std::string> spineTocId(
    std::string_view opf
) {
    const std::string lower_opf = lower(opf);
    const auto pos =
        lower_opf.find("<spine");

    if (pos == std::string::npos) {
        return std::nullopt;
    }

    const auto end =
        lower_opf.find('>', pos);

    if (end == std::string::npos) {
        return std::nullopt;
    }

    return attribute(
        opf.substr(
            pos,
            end - pos + 1U
        ),
        "toc"
    );
}

std::map<std::string, std::string>
parseNcxLabels(
    std::string_view ncx,
    std::string_view ncx_directory
) {
    std::map<std::string, std::string> labels;
    const std::string lower_ncx = lower(ncx);
    std::size_t cursor = 0;

    while (true) {
        const auto point =
            lower_ncx.find(
                "<navpoint",
                cursor
            );

        if (point == std::string::npos) {
            break;
        }

        const auto point_end =
            lower_ncx.find(
                "</navpoint>",
                point
            );

        if (point_end == std::string::npos) {
            break;
        }

        const auto segment =
            std::string_view(ncx).substr(
                point,
                point_end - point
            );

        const auto label =
            firstTagText(
                segment,
                "text"
            );

        const std::string lower_segment =
            lower(segment);

        const auto content_pos =
            lower_segment.find(
                "<content"
            );

        if (content_pos !=
            std::string::npos) {
            const auto content_end =
                lower_segment.find(
                    '>',
                    content_pos
                );

            if (content_end !=
                std::string::npos) {
                const auto tag =
                    segment.substr(
                        content_pos,
                        content_end -
                            content_pos + 1U
                    );

                const auto src =
                    attribute(tag, "src");

                if (src.has_value() &&
                    label.has_value() &&
                    !label->empty()) {
                    auto path = *src;
                    const auto hash =
                        path.find('#');

                    if (hash !=
                        std::string::npos) {
                        path.resize(hash);
                    }

                    if (!path.empty()) {
                        labels[
                            normalizePath(
                                ncx_directory,
                                path
                            )
                        ] = *label;
                    }
                }
            }
        }

        cursor =
            point_end + 11U;
    }

    return labels;
}

std::map<std::string, std::string>
parseNavLabels(
    std::string_view nav_xhtml,
    std::string_view nav_directory
) {
    std::map<std::string, std::string> labels;
    const std::string lower_nav =
        lower(nav_xhtml);
    std::size_t cursor = 0;

    while (true) {
        const auto anchor =
            lower_nav.find(
                "<a",
                cursor
            );

        if (anchor == std::string::npos) {
            break;
        }

        const auto open_end =
            lower_nav.find('>', anchor);

        if (open_end == std::string::npos) {
            break;
        }

        const auto close =
            lower_nav.find(
                "</a>",
                open_end + 1U
            );

        if (close == std::string::npos) {
            break;
        }

        const auto tag =
            nav_xhtml.substr(
                anchor,
                open_end - anchor + 1U
            );

        const auto href =
            attribute(tag, "href");

        if (href.has_value()) {
            auto path = *href;
            const auto hash = path.find('#');

            if (hash != std::string::npos) {
                path.resize(hash);
            }

            const auto label =
                stripInlineTags(
                    nav_xhtml.substr(
                        open_end + 1U,
                        close - open_end - 1U
                    )
                );

            if (!path.empty() &&
                !label.empty()) {
                labels[
                    normalizePath(
                        nav_directory,
                        path
                    )
                ] = label;
            }
        }

        cursor = close + 4U;
    }

    return labels;
}

std::string filenameStem(
    std::string_view filename
) {
    const auto slash =
        filename.find_last_of("/\\");
    const auto base =
        slash == std::string_view::npos
            ? filename
            : filename.substr(slash + 1U);
    const auto dot = base.find_last_of('.');

    return std::string(
        dot == std::string_view::npos
            ? base
            : base.substr(0, dot)
    );
}

struct ManifestItem {
    std::string href;
    std::string media_type;
    std::string properties;
};

bool parseManifest(
    std::string_view opf,
    std::map<std::string, ManifestItem>& manifest,
    bool& has_nav
) {
    const std::string lower_opf = lower(opf);
    std::size_t cursor = 0;

    while (true) {
        const auto pos =
            lower_opf.find("<item", cursor);

        if (pos == std::string::npos) {
            break;
        }

        const auto after =
            pos + 5U;

        if (after < lower_opf.size() &&
            lower_opf[after] != ' ' &&
            lower_opf[after] != '\t' &&
            lower_opf[after] != '\r' &&
            lower_opf[after] != '\n' &&
            lower_opf[after] != '>') {
            cursor = after;
            continue;
        }

        const auto end =
            lower_opf.find('>', after);

        if (end == std::string::npos) {
            return false;
        }

        const auto tag =
            opf.substr(
                pos,
                end - pos + 1U
            );

        const auto id = attribute(tag, "id");
        const auto href = attribute(tag, "href");

        if (id.has_value() &&
            href.has_value()) {
            ManifestItem item;
            item.href = *href;

            if (auto value =
                    attribute(
                        tag,
                        "media-type"
                    );
                value.has_value()) {
                item.media_type = *value;
            }

            if (auto value =
                    attribute(
                        tag,
                        "properties"
                    );
                value.has_value()) {
                item.properties = *value;

                if (lower(item.properties).
                        find("nav") !=
                    std::string::npos) {
                    has_nav = true;
                }
            }

            manifest[*id] =
                std::move(item);
        }

        cursor = end + 1U;
    }

    return !manifest.empty();
}

std::vector<std::string> parseSpine(
    std::string_view opf
) {
    std::vector<std::string> ids;
    const std::string lower_opf = lower(opf);
    std::size_t cursor = 0;

    while (true) {
        const auto pos =
            lower_opf.find(
                "<itemref",
                cursor
            );

        if (pos == std::string::npos) {
            break;
        }

        const auto end =
            lower_opf.find('>', pos);

        if (end == std::string::npos) {
            break;
        }

        const auto tag =
            opf.substr(
                pos,
                end - pos + 1U
            );

        const auto linear =
            attribute(tag, "linear");

        const bool include =
            !linear.has_value() ||
            lower(*linear) != "no";

        if (include) {
            if (auto idref =
                    attribute(tag, "idref");
                idref.has_value()) {
                ids.push_back(*idref);
            }
        }

        cursor = end + 1U;
    }

    return ids;
}

} // namespace

namespace {

ParseResult parseEpubArchive(
    ZipArchive& archive,
    const ParserSourceInfo& source,
    bool metadata_only
) {
    ParseResult result;
    result.document.book_id = source.book_id;

    if (archive.status() !=
        ZipArchiveStatus::Ok) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    std::string container_xml;

    if (archive.read(
            "META-INF/container.xml",
            container_xml
        ) != ZipArchiveStatus::Ok) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    const auto lower_container =
        lower(container_xml);

    std::optional<std::size_t>
        rootfile_position;
    std::size_t rootfile_search = 0;

    while (true) {
        const auto candidate =
            lower_container.find(
                "<rootfile",
                rootfile_search
            );

        if (candidate ==
            std::string::npos) {
            break;
        }

        const auto boundary =
            candidate + 9U;

        if (boundary >=
                lower_container.size() ||
            lower_container[boundary] == '>' ||
            lower_container[boundary] == '/' ||
            std::isspace(
                static_cast<unsigned char>(
                    lower_container[boundary]
                )
            )) {
            rootfile_position = candidate;
            break;
        }

        rootfile_search = boundary;
    }

    if (!rootfile_position.has_value()) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    const auto rootfile_pos =
        *rootfile_position;

    const auto rootfile_end =
        container_xml.find(
            '>',
            rootfile_pos
        );

    if (rootfile_end ==
        std::string::npos) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    const auto rootfile_tag =
        std::string_view(container_xml).
            substr(
                rootfile_pos,
                rootfile_end -
                    rootfile_pos + 1U
            );

    const auto opf_path =
        attribute(
            rootfile_tag,
            "full-path"
        );

    if (!opf_path.has_value() ||
        opf_path->empty()) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    std::string opf;

    if (archive.read(
            *opf_path,
            opf
        ) != ZipArchiveStatus::Ok) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    result.document.metadata.title =
        dcText(opf, "title").
            value_or(
                filenameStem(
                    source.source_filename
                )
            );

    const auto creators =
        dcTexts(opf, "creator");

    result.document.metadata.authors =
        creators;

    if (creators.empty()) {
        result.document.metadata.author_display =
            "Unknown author";
    } else {
        result.document.metadata.author_display =
            creators.front();

        for (std::size_t i = 1;
             i < creators.size();
             ++i) {
            result.document.metadata.author_display +=
                ", " + creators[i];
        }
    }

    result.document.metadata.language =
        dcText(opf, "language");
    result.document.metadata.identifier =
        dcText(opf, "identifier");
    result.document.metadata.description =
        dcText(opf, "description");
    result.document.metadata.publisher =
        dcText(opf, "publisher");
    result.document.metadata.published_date =
        dcText(opf, "date");

    std::map<std::string, ManifestItem>
        manifest;
    bool has_nav = false;

    if (!parseManifest(
            opf,
            manifest,
            has_nav
        )) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    if (metadata_only) {
        result.document.metadata.toc_available =
            has_nav ||
            spineTocId(opf).has_value();

        result.status = ParserStatus::Ok;
        return result;
    }

    const auto spine = parseSpine(opf);

    if (spine.empty()) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    const auto opf_directory =
        directoryOf(*opf_path);

    std::map<std::string, std::string>
        nav_labels;

    if (has_nav) {
        const auto nav_item =
            std::find_if(
                manifest.begin(),
                manifest.end(),
                [](const auto& pair) {
                    return lower(
                        pair.second.properties
                    ).find("nav") !=
                        std::string::npos;
                }
            );

        if (nav_item != manifest.end()) {
            const auto nav_path =
                normalizePath(
                    opf_directory,
                    nav_item->second.href
                );

            std::string nav_xhtml;

            if (archive.read(
                    nav_path,
                    nav_xhtml
                ) == ZipArchiveStatus::Ok) {
                nav_labels =
                    parseNavLabels(
                        nav_xhtml,
                        directoryOf(nav_path)
                    );
            }
        }
    }

    if (nav_labels.empty()) {
        const auto toc_id =
            spineTocId(opf);

        if (toc_id.has_value()) {
            const auto toc_item =
                manifest.find(*toc_id);

            if (toc_item != manifest.end()) {
                const auto ncx_path =
                    normalizePath(
                        opf_directory,
                        toc_item->second.href
                    );

                std::string ncx;

                if (archive.read(
                        ncx_path,
                        ncx
                    ) == ZipArchiveStatus::Ok) {
                    nav_labels =
                        parseNcxLabels(
                            ncx,
                            directoryOf(ncx_path)
                        );
                }
            }
        }
    }

    result.document.metadata.toc_available =
        !nav_labels.empty() || has_nav;

    std::uint64_t global_offset = 0;

    for (const auto& idref : spine) {
        const auto item =
            manifest.find(idref);

        if (item == manifest.end()) {
            continue;
        }

        const auto media_type =
            lower(item->second.media_type);

        if (!media_type.empty() &&
            media_type !=
                "application/xhtml+xml" &&
            media_type !=
                "text/html") {
            continue;
        }

        const auto path =
            normalizePath(
                opf_directory,
                item->second.href
            );

        std::string xhtml;

        if (archive.read(
                path,
                xhtml
            ) != ZipArchiveStatus::Ok) {
            continue;
        }

        const auto blocks =
            parseXhtmlBlocks(xhtml);

        if (blocks.empty()) {
            continue;
        }

        DocumentSection section;
        section.id = path;

        if (const auto nav =
                nav_labels.find(path);
            nav != nav_labels.end()) {
            section.title = nav->second;
        } else {
            const auto heading =
                std::find_if(
                    blocks.begin(),
                    blocks.end(),
                    [](const ParsedBlock& block) {
                        return block.type ==
                            TextBlockType::Heading;
                    }
                );

            if (heading != blocks.end()) {
                section.title =
                    heading->text;
            }
        }

        std::uint64_t section_offset = 0;

        for (const auto& parsed_block :
             blocks) {
            TextBlock block;
            block.type =
                parsed_block.type;
            block.text =
                parsed_block.text;
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

    if (result.document.sections.empty()) {
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    result.document.total_text_length =
        global_offset;
    result.status = ParserStatus::Ok;
    return result;
}

} // namespace

BookFormat EpubParser::format() const {
    return BookFormat::Epub;
}

ParseResult EpubParser::parseMetadata(
    std::string_view bytes,
    const ParserSourceInfo& source
) const {
    if (bytes.empty()) {
        ParseResult result;
        result.document.book_id =
            source.book_id;
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    ZipArchive archive(bytes);
    return parseEpubArchive(
        archive,
        source,
        true
    );
}

ParseResult EpubParser::parse(
    std::string_view bytes,
    const ParserSourceInfo& source
) const {
    if (bytes.empty()) {
        ParseResult result;
        result.document.book_id =
            source.book_id;
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    ZipArchive archive(bytes);
    return parseEpubArchive(
        archive,
        source,
        false
    );
}

ParseResult EpubParser::parse(
    const ZipRangeSource& source_bytes,
    const ParserSourceInfo& source
) const {
    if (source_bytes.size() == 0U) {
        ParseResult result;
        result.document.book_id =
            source.book_id;
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    ZipArchive archive(source_bytes);
    return parseEpubArchive(
        archive,
        source,
        false
    );
}

ParseResult EpubParser::parseMetadata(
    const ZipRangeSource& source_bytes,
    const ParserSourceInfo& source
) const {
    if (source_bytes.size() == 0U) {
        ParseResult result;
        result.document.book_id =
            source.book_id;
        result.status =
            ParserStatus::EmptyDocument;
        return result;
    }

    ZipArchive archive(source_bytes);
    return parseEpubArchive(
        archive,
        source,
        true
    );
}

} // namespace enku
