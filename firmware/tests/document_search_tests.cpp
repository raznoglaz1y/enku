#include <cassert>
#include <string>

#include "enku/reader/document_search.hpp"

using namespace enku;

int main() {
    BookDocument document;
    document.book_id = "search-test";
    document.sections = {
        DocumentSection{
            "chapter-1",
            std::string("Chapter 1"),
            {
                TextBlock{
                    TextBlockType::Paragraph,
                    "Alpha beta. Привет мир. beta.",
                    0,
                },
            },
            0,
        },
        DocumentSection{
            "chapter-2",
            std::nullopt,
            {
                TextBlock{
                    TextBlockType::Paragraph,
                    "BETA gamma. привет.",
                    100,
                },
            },
            0,
        },
    };

    DocumentSearch search;

    assert(search.count(document, "beta") == 3);
    assert(search.count(document, "BETA") == 3);
    assert(search.count(document, "ПРИВЕТ") == 2);
    assert(search.count(document, "") == 0);

    const auto first =
        search.window(
            document,
            "beta",
            0,
            2
        );

    assert(first.total_matches == 3);
    assert(first.window_start == 0);
    assert(first.matches.size() == 2);
    assert(first.matches[0].section_label == "Chapter 1");
    assert(first.matches[0].position.book_id == "search-test");
    assert(
        first.matches[0].preview.substr(
            first.matches[0].preview_match_start,
            first.matches[0].preview_match_length
        ) == "beta"
    );

    const auto second =
        search.window(
            document,
            "beta",
            2,
            2
        );

    assert(second.total_matches == 3);
    assert(second.window_start == 2);
    assert(second.matches.size() == 1);
    assert(second.matches[0].section_label == "chapter-2");
    assert(second.matches[0].position.text_offset == 100);

    const auto cyr =
        search.window(
            document,
            "ПРИВЕТ",
            0,
            10
        );

    assert(cyr.total_matches == 2);
    assert(cyr.matches.size() == 2);
    assert(
        cyr.matches[0].preview.substr(
            cyr.matches[0].preview_match_start,
            cyr.matches[0].preview_match_length
        ) == "Привет"
    );

    const auto empty =
        search.window(
            document,
            "missing",
            0,
            10
        );

    assert(empty.total_matches == 0);
    assert(empty.matches.empty());

    return 0;
}
