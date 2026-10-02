#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../core/refresh.hpp"

namespace enku {

class MonoFramebufferSource {
public:
    virtual ~MonoFramebufferSource() = default;

    virtual std::uint16_t width() const = 0;
    virtual std::uint16_t height() const = 0;

    virtual const std::uint8_t* data() const = 0;
    virtual std::size_t size() const = 0;

    virtual bool copyRegion(
        const Rect& region,
        std::vector<std::uint8_t>& bytes
    ) const = 0;
};

} // namespace enku
