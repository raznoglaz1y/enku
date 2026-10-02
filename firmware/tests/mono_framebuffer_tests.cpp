#include "enku/render/owned_mono_framebuffer.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace enku;

namespace {

void setBlack(
    OwnedMonoFramebuffer& framebuffer,
    std::uint16_t x,
    std::uint16_t y
) {
    const std::size_t row_bytes =
        (framebuffer.width() + 7U) / 8U;
    const std::size_t index =
        static_cast<std::size_t>(y) * row_bytes +
        x / 8U;

    framebuffer.mutableData()[index] &=
        static_cast<std::uint8_t>(
            ~(0x80U >> (x & 7U))
        );
}

} // namespace

int main() {
    OwnedMonoFramebuffer framebuffer(16, 8);

    assert(framebuffer.width() == 16);
    assert(framebuffer.height() == 8);
    assert(framebuffer.size() == 16);

    framebuffer.clearWhite();
    framebuffer.setBlack(2, 2);
    framebuffer.setWhite(2, 2);
    framebuffer.setBlack(-1, -1);
    framebuffer.setWhite(999, 999);

    for (std::size_t i = 0; i < framebuffer.size(); ++i) {
        assert(framebuffer.data()[i] == 0xFF);
    }

    setBlack(framebuffer, 0, 0);
    setBlack(framebuffer, 7, 0);
    setBlack(framebuffer, 8, 0);
    setBlack(framebuffer, 15, 7);

    std::vector<std::uint8_t> region;

    assert(
        framebuffer.copyRegion(
            Rect{0, 0, 8, 1},
            region
        )
    );
    assert(region.size() == 1);
    assert(region[0] == 0x7E);

    assert(
        framebuffer.copyRegion(
            Rect{8, 0, 8, 1},
            region
        )
    );
    assert(region.size() == 1);
    assert(region[0] == 0x7F);

    assert(
        framebuffer.copyRegion(
            Rect{3, 0, 10, 2},
            region
        )
    );
    // Region extraction is byte-aligned for the panel window:
    // x=3,width=10 spans source bytes 0..1 on each row.
    assert(region.size() == 4);
    assert(region[0] == 0x7E);
    assert(region[1] == 0x7F);
    assert(region[2] == 0xFF);
    assert(region[3] == 0xFF);

    assert(
        !framebuffer.copyRegion(
            Rect{16, 0, 1, 1},
            region
        )
    );

    framebuffer.clearBlack();

    for (std::size_t i = 0; i < framebuffer.size(); ++i) {
        assert(framebuffer.data()[i] == 0x00);
    }

    return 0;
}
