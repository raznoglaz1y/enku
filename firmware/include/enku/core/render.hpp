#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "refresh.hpp"

namespace enku {

class MonoFramebuffer {
public:
    MonoFramebuffer(
        std::uint16_t width,
        std::uint16_t height
    );

    std::uint16_t width() const;
    std::uint16_t height() const;
    std::size_t size() const;

    std::uint8_t* data();
    const std::uint8_t* data() const;

    void clearWhite();
    void clearBlack();

    bool extractRegion(
        const Rect& rect,
        std::vector<std::uint8_t>& bytes
    ) const;

private:
    std::uint16_t width_{0};
    std::uint16_t height_{0};
    std::vector<std::uint8_t> bytes_;
};

} // namespace enku
