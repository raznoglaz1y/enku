#pragma once

#include <optional>

#include "reader_types.hpp"

namespace enku {

class ReaderEngine {
public:
    virtual ~ReaderEngine() = default;

    virtual std::optional<PageResult> layoutPage(
        const LayoutRequest& request
    ) = 0;
};

} // namespace enku
