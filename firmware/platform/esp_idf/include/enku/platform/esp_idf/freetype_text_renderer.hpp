#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "freetype/freetype.h"

#include "enku/reader/pagination.hpp"
#include "enku/reader/reader_types.hpp"
#include "enku/render/owned_mono_framebuffer.hpp"

namespace enku::platform::esp_idf {

enum class FontInitStatus : std::uint8_t {
    Ok,
    LibraryInitFailed,
    FontNotFound,
    FaceLoadFailed,
    SizeFailed,
};

class FreeTypeTextRenderer final : public TextMeasurer {
public:
    explicit FreeTypeTextRenderer(
        std::string font_path =
            "/sdcard/system/fonts/NotoSans-Regular.ttf"
    );

    ~FreeTypeTextRenderer();

    FontInitStatus begin(
        std::uint16_t initial_size_px = 18
    );

    bool ready() const;

    std::uint16_t measureWidthPx(
        std::string_view utf8,
        const TypographySettings& typography
    ) const override;

    std::uint16_t lineHeightPx(
        const TypographySettings& typography
    ) const override;

    bool renderPage(
        const PageResult& page,
        const TypographySettings& typography,
        OwnedMonoFramebuffer& framebuffer
    ) const;

private:
    std::string font_path_;

    mutable FT_Library library_{nullptr};
    mutable FT_Face face_{nullptr};
    mutable std::uint16_t current_size_px_{0};

    bool ensureSize(
        std::uint16_t size_px
    ) const;

    static bool nextCodepoint(
        std::string_view text,
        std::size_t& offset,
        std::uint32_t& codepoint
    );

    static void drawMonoBitmap(
        OwnedMonoFramebuffer& framebuffer,
        const FT_Bitmap& bitmap,
        int x,
        int y
    );
};

} // namespace enku::platform::esp_idf
