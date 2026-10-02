#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "document.hpp"

namespace enku {

enum class ParserStatus : std::uint8_t {
    Ok,
    EmptyDocument,
    InvalidUtf8,
    UnsupportedEncoding,
    InvalidSource,
};

struct ParserSourceInfo {
    BookId book_id;
    std::string source_path;
    std::string source_filename;
};

struct ParseResult {
    ParserStatus status{ParserStatus::InvalidSource};
    BookDocument document;

    bool ok() const {
        return status == ParserStatus::Ok;
    }
};

class BookParser {
public:
    virtual ~BookParser() = default;

    virtual BookFormat format() const = 0;
    virtual ParseResult parse(
        std::string_view bytes,
        const ParserSourceInfo& source
    ) const = 0;
};

} // namespace enku
