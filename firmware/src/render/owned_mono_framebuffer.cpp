#include "enku/render/owned_mono_framebuffer.hpp"

#include <algorithm>
#include <cstddef>

namespace enku {

OwnedMonoFramebuffer::OwnedMonoFramebuffer(
    std::uint16_t width,
    std::uint16_t height
)
    : width_(width),
      height_(height),
      bytes_(
          (static_cast<std::size_t>(width) *
           static_cast<std::size_t>(height) +
           7U) / 8U,
          0xFF
      ) {}

std::uint16_t OwnedMonoFramebuffer::width() const {
    return width_;
}

std::uint16_t OwnedMonoFramebuffer::height() const {
    return height_;
}

const std::uint8_t*
OwnedMonoFramebuffer::data() const {
    return bytes_.data();
}

std::size_t OwnedMonoFramebuffer::size() const {
    return bytes_.size();
}

std::uint8_t*
OwnedMonoFramebuffer::mutableData() {
    return bytes_.data();
}

void OwnedMonoFramebuffer::setBlack(
    int x,
    int y
) {
    if (x < 0 || y < 0 ||
        x >= width_ || y >= height_) {
        return;
    }

    const std::size_t row_bytes =
        (static_cast<std::size_t>(width_) + 7U) / 8U;

    const std::size_t index =
        static_cast<std::size_t>(y) * row_bytes +
        static_cast<std::size_t>(x / 8);

    bytes_[index] &=
        static_cast<std::uint8_t>(
            ~(0x80U >> (x & 7))
        );
}

void OwnedMonoFramebuffer::setWhite(
    int x,
    int y
) {
    if (x < 0 || y < 0 ||
        x >= width_ || y >= height_) {
        return;
    }

    const std::size_t row_bytes =
        (static_cast<std::size_t>(width_) + 7U) / 8U;

    const std::size_t index =
        static_cast<std::size_t>(y) * row_bytes +
        static_cast<std::size_t>(x / 8);

    bytes_[index] |=
        static_cast<std::uint8_t>(
            0x80U >> (x & 7)
        );
}

void OwnedMonoFramebuffer::clearWhite() {
    std::fill(
        bytes_.begin(),
        bytes_.end(),
        0xFF
    );
}

void OwnedMonoFramebuffer::clearBlack() {
    std::fill(
        bytes_.begin(),
        bytes_.end(),
        0x00
    );
}

bool OwnedMonoFramebuffer::copyRegion(
    const Rect& region,
    std::vector<std::uint8_t>& bytes
) const {
    bytes.clear();

    if (region.width == 0 ||
        region.height == 0 ||
        region.x >= width_ ||
        region.y >= height_ ||
        static_cast<std::uint32_t>(region.x) +
            region.width > width_ ||
        static_cast<std::uint32_t>(region.y) +
            region.height > height_) {
        return false;
    }

    const std::uint16_t x_start_byte =
        region.x / 8U;

    const std::uint16_t x_end_byte =
        static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(region.x) +
             region.width +
             7U) / 8U
        );

    const std::uint16_t region_row_bytes =
        static_cast<std::uint16_t>(
            x_end_byte - x_start_byte
        );

    const std::size_t source_row_bytes =
        (static_cast<std::size_t>(width_) + 7U) / 8U;

    bytes.resize(
        static_cast<std::size_t>(region_row_bytes) *
        region.height
    );

    for (std::uint16_t row = 0;
         row < region.height;
         ++row) {
        const std::size_t source_offset =
            static_cast<std::size_t>(
                region.y + row
            ) * source_row_bytes +
            x_start_byte;

        const std::size_t target_offset =
            static_cast<std::size_t>(row) *
            region_row_bytes;

        std::copy_n(
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(
                    source_offset
                ),
            region_row_bytes,
            bytes.begin() +
                static_cast<std::ptrdiff_t>(
                    target_offset
                )
        );
    }

    return true;
}

} // namespace enku
