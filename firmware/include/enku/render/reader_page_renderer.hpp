#pragma once

#include "../reader/reader_types.hpp"

namespace enku {

class ReaderPageRenderer {
public:
    virtual ~ReaderPageRenderer() = default;

    virtual bool renderPage(
        const PageResult& page,
        const TypographySettings& typography
    ) = 0;
};

} // namespace enku
