#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "freetype/freetype.h"

#include "enku/reader/pagination.hpp"
#include "enku/reader/reader_types.hpp"
#include "enku/render/owned_mono_framebuffer.hpp"
#include "enku/render/reader_page_renderer.hpp"
#include "enku/render/library_page_renderer.hpp"
#include "enku/render/reader_overlay_renderer.hpp"

namespace enku::platform::esp_idf {

enum class FontInitStatus : std::uint8_t {
    Ok,
    LibraryInitFailed,
    FontNotFound,
    FaceLoadFailed,
    SizeFailed,
};

class FreeTypeTextRenderer final
    : public TextMeasurer,
      public ReaderPageRenderer,
      public LibraryPageRenderer,
      public ReaderOverlayRenderer {
public:
    FreeTypeTextRenderer(
        OwnedMonoFramebuffer& framebuffer,
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
        Orientation orientation
    ) override;

    bool renderLibrary(
        const AppState& app_state,
        const LibraryPage& page
    ) override;

    bool renderReaderOverlay(
        const AppState& app_state
    ) override;

private:
    OwnedMonoFramebuffer& framebuffer_;
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

    void drawMonoBitmap(
        const FT_Bitmap& bitmap,
        int x,
        int y,
        Orientation orientation
    );

    void setLogicalBlack(
        int x,
        int y,
        Orientation orientation
    );

    bool drawTextAt(
        std::string_view text,
        std::uint16_t size_px,
        int x,
        int baseline,
        Orientation orientation
    );

    void drawRect(
        int x,
        int y,
        int width,
        int height,
        Orientation orientation,
        int thickness = 1
    );
};

} // namespace enku::platform::esp_idf
