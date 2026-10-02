#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "mono_framebuffer.hpp"

namespace enku {

class OwnedMonoFramebuffer final
    : public MonoFramebufferSource {
public:
    OwnedMonoFramebuffer(
        std::uint16_t width,
        std::uint16_t height
    );

    std::uint16_t width() const override;
    std::uint16_t height() const override;

    const std::uint8_t* data() const override;
    std::size_t size() const override;

    bool copyRegion(
        const Rect& region,
        std::vector<std::uint8_t>& bytes
    ) const override;

    std::uint8_t* mutableData();

    void clearWhite();
    void clearBlack();

private:
    std::uint16_t width_{0};
    std::uint16_t height_{0};
    std::vector<std::uint8_t> bytes_;
};

} // namespace enku
