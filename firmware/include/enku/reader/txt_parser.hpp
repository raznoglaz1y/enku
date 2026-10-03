#pragma once

#include "parser.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace enku {

class TextRangeSource {
public:
    virtual ~TextRangeSource() = default;

    virtual std::uint64_t size() const = 0;

    virtual bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const = 0;
};

class TxtParser final : public BookParser {
public:
    BookFormat format() const override;

    ParseResult parse(
        std::string_view bytes,
        const ParserSourceInfo& source
    ) const override;

    ParseResult parse(
        const TextRangeSource& source_bytes,
        const ParserSourceInfo& source
    ) const;
};

} // namespace enku
