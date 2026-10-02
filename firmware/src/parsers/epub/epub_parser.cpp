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

std::string stripXmlTags(
    std::string_view markup
) {
    std::string text;
    text.reserve(markup.size());

    bool in_tag = false;
    bool in_script = false;
    bool in_style = false;
    std::string tag;

    const auto flushTag =
        [&]() {
            const auto lowered = lower(trim(tag));

            const bool closing =
                !lowered.empty() &&
                lowered.front() == '/';

            std::string name = lowered;

            if (closing) {
                name.erase(name.begin());
            }

            const auto space =
                name.find_first_of(" \t\r\n/>");

            if (space != std::string::npos) {
                name.resize(space);
            }

            if (name == "script") {
                in_script = !closing;
            } else if (name == "style") {
                in_style = !closing;
            }

            if (!in_script &&
                !in_style &&
                (name == "p" ||
                 name == "div" ||
                 name == "section" ||
                 name == "article" ||
                 name == "li" ||
                 name == "blockquote" ||
                 name == "br" ||
                 name == "h1" ||
                 name == "h2" ||
                 name == "h3" ||
                 name == "h4" ||
                 name == "h5" ||
                 name == "h6")) {
                if (!text.empty() &&
                    text.back() != '\n') {
                    text.push_back('\n');
                }
            }

            tag.clear();
        };

    for (const char ch : markup) {
        if (!in_tag) {
            if (ch == '<') {
                in_tag = true;
                tag.clear();
                continue;
            }

            if (!in_script && !in_style) {
                text.push_back(ch);
            }
            continue;
        }

        if (ch == '>') {
            in_tag = false;
            flushTag();
        } else {
            tag.push_back(ch);
        }
    }

    return xmlDecode(text);
}

std::vector<std::string> paragraphs(
    std::string_view text
) {
    std::vector<std::string> out;
    std::string current;
    bool previous_space = false;

    const auto flush =
        [&]() {
            const auto clean = trim(current);
            current.clear();
            previous_space = false;

            if (!clean.empty()) {
                out.push_back(clean);
            }
        };

    for (const char ch : text) {
        if (ch == '\n') {
            flush();
            continue;
        }

        const bool space =
            std::isspace(
                static_cast<unsigned char>(
                    ch
                )
            );

        if (space) {
            if (!previous_space &&
                !current.empty()) {
                current.push_back(' ');
            }
        } else {
            current.push_back(ch);
        }

        previous_space = space;
    }

    flush();
    return out;
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

        if (auto idref =
                attribute(tag, "idref");
            idref.has_value()) {
            ids.push_back(*idref);
        }

        cursor = end + 1U;
    }

    return ids;
}

} // namespace

BookFormat EpubParser::format() const {
    return BookFormat::Epub;
}

ParseResult EpubParser::parse(
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

    ZipArchive archive(bytes);

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

    const auto rootfile_pos =
        lower(container_xml).find(
            "<rootfile"
        );

    if (rootfile_pos ==
        std::string::npos) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

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

    const auto creator =
        dcText(opf, "creator");

    result.document.metadata.author_display =
        creator.has_value() &&
                !creator->empty()
            ? *creator
            : "Unknown author";

    if (creator.has_value() &&
        !creator->empty()) {
        result.document.metadata.authors.
            push_back(*creator);
    }

    result.document.metadata.language =
        dcText(opf, "language");
    result.document.metadata.identifier =
        dcText(opf, "identifier");

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

    const auto spine = parseSpine(opf);

    if (spine.empty()) {
        result.status =
            ParserStatus::InvalidSource;
        return result;
    }

    result.document.metadata.toc_available =
        has_nav;

    const auto opf_directory =
        directoryOf(*opf_path);

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

        const auto clean_text =
            stripXmlTags(xhtml);
        const auto blocks =
            paragraphs(clean_text);

        if (blocks.empty()) {
            continue;
        }

        DocumentSection section;
        section.id = path;

        std::uint64_t section_offset = 0;

        for (const auto& paragraph :
             blocks) {
            TextBlock block;
            block.type =
                TextBlockType::Paragraph;
            block.text = paragraph;
            block.text_offset =
                global_offset +
                section_offset;

            section_offset +=
                static_cast<std::uint64_t>(
                    paragraph.size()
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

} // namespace enku
