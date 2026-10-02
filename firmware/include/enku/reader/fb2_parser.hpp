#pragma once

#include "parser.hpp"

namespace enku {

class Fb2Parser final : public BookParser {
public:
    BookFormat format() const override;

    ParseResult parse(
        std::string_view bytes,
        const ParserSourceInfo& source
    ) const override;
};

} // namespace enku
