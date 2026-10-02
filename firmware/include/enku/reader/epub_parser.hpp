#pragma once

#include "parser.hpp"

namespace enku {

class ZipRangeSource;

class EpubParser final : public BookParser {
public:
    BookFormat format() const override;

    ParseResult parse(
        std::string_view bytes,
        const ParserSourceInfo& source
    ) const override;

    ParseResult parseMetadata(
        std::string_view bytes,
        const ParserSourceInfo& source
    ) const;

    ParseResult parse(
        const ZipRangeSource& source_bytes,
        const ParserSourceInfo& source
    ) const;

    ParseResult parseMetadata(
        const ZipRangeSource& source_bytes,
        const ParserSourceInfo& source
    ) const;
};

} // namespace enku
