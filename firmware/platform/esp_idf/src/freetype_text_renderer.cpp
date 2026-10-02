#include "enku/platform/esp_idf/freetype_text_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "esp_log.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_FONT";

} // namespace

FreeTypeTextRenderer::FreeTypeTextRenderer(
    std::string font_path
)
    : font_path_(std::move(font_path)) {}

FreeTypeTextRenderer::~FreeTypeTextRenderer() {
    if (face_ != nullptr) {
        FT_Done_Face(face_);
        face_ = nullptr;
    }

    if (library_ != nullptr) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
    }
}

FontInitStatus FreeTypeTextRenderer::begin(
    std::uint16_t initial_size_px
) {
    FILE* probe = std::fopen(
        font_path_.c_str(),
        "rb"
    );

    if (probe == nullptr) {
        ESP_LOGE(
            kTag,
            "Font not found: %s",
            font_path_.c_str()
        );
        return FontInitStatus::FontNotFound;
    }

    std::fclose(probe);

    if (FT_Init_FreeType(&library_) != 0) {
        return FontInitStatus::LibraryInitFailed;
    }

    if (FT_New_Face(
            library_,
            font_path_.c_str(),
            0,
            &face_
        ) != 0) {
        return FontInitStatus::FaceLoadFailed;
    }

    if (!ensureSize(initial_size_px)) {
        return FontInitStatus::SizeFailed;
    }

    ESP_LOGI(
        kTag,
        "Loaded %s (%s)",
        face_->family_name != nullptr
            ? face_->family_name
            : "unknown family",
        face_->style_name != nullptr
            ? face_->style_name
            : "unknown style"
    );

    return FontInitStatus::Ok;
}

bool FreeTypeTextRenderer::ready() const {
    return library_ != nullptr &&
           face_ != nullptr;
}

bool FreeTypeTextRenderer::ensureSize(
    std::uint16_t size_px
) const {
    if (face_ == nullptr || size_px == 0) {
        return false;
    }

    if (current_size_px_ == size_px) {
        return true;
    }

    if (FT_Set_Pixel_Sizes(
            face_,
            0,
            size_px
        ) != 0) {
        return false;
    }

    current_size_px_ = size_px;
    return true;
}

bool FreeTypeTextRenderer::nextCodepoint(
    std::string_view text,
    std::size_t& offset,
    std::uint32_t& codepoint
) {
    if (offset >= text.size()) {
        return false;
    }

    const auto first =
        static_cast<std::uint8_t>(
            text[offset]
        );

    if (first < 0x80U) {
        codepoint = first;
        ++offset;
        return true;
    }

    std::size_t count = 0;
    std::uint32_t value = 0;

    if ((first & 0xE0U) == 0xC0U) {
        count = 2;
        value = first & 0x1FU;
    } else if ((first & 0xF0U) == 0xE0U) {
        count = 3;
        value = first & 0x0FU;
    } else if ((first & 0xF8U) == 0xF0U) {
        count = 4;
        value = first & 0x07U;
    } else {
        ++offset;
        codepoint = 0xFFFDU;
        return true;
    }

    if (offset + count > text.size()) {
        offset = text.size();
        codepoint = 0xFFFDU;
        return true;
    }

    for (std::size_t i = 1; i < count; ++i) {
        const auto continuation =
            static_cast<std::uint8_t>(
                text[offset + i]
            );

        if ((continuation & 0xC0U) != 0x80U) {
            ++offset;
            codepoint = 0xFFFDU;
            return true;
        }

        value =
            (value << 6U) |
            (continuation & 0x3FU);
    }

    offset += count;
    codepoint = value;
    return true;
}

std::uint16_t FreeTypeTextRenderer::measureWidthPx(
    std::string_view utf8,
    const TypographySettings& typography
) const {
    if (!ensureSize(typography.font_size_px)) {
        return 0;
    }

    FT_UInt previous_glyph = 0;
    std::int64_t width_26_6 = 0;

    std::size_t offset = 0;
    std::uint32_t codepoint = 0;

    while (nextCodepoint(
        utf8,
        offset,
        codepoint
    )) {
        const FT_UInt glyph =
            FT_Get_Char_Index(
                face_,
                static_cast<FT_ULong>(
                    codepoint
                )
            );

        if (FT_HAS_KERNING(face_) &&
            previous_glyph != 0 &&
            glyph != 0) {
            FT_Vector kerning = {};
            if (FT_Get_Kerning(
                    face_,
                    previous_glyph,
                    glyph,
                    FT_KERNING_DEFAULT,
                    &kerning
                ) == 0) {
                width_26_6 += kerning.x;
            }
        }

        if (FT_Load_Glyph(
                face_,
                glyph,
                FT_LOAD_DEFAULT
            ) != 0) {
            previous_glyph = 0;
            continue;
        }

        width_26_6 +=
            face_->glyph->advance.x;
        previous_glyph = glyph;
    }

    const auto pixels =
        static_cast<std::int64_t>(
            (width_26_6 + 63) >> 6
        );

    return static_cast<std::uint16_t>(
        std::min<std::int64_t>(
            pixels,
            std::numeric_limits<std::uint16_t>::max()
        )
    );
}

std::uint16_t FreeTypeTextRenderer::lineHeightPx(
    const TypographySettings& typography
) const {
    if (!ensureSize(typography.font_size_px)) {
        return 0;
    }

    const auto natural_height =
        static_cast<float>(
            face_->size->metrics.height
        ) / 64.0F;

    const auto requested =
        natural_height *
        typography.line_spacing;

    return static_cast<std::uint16_t>(
        std::max(
            1.0F,
            std::ceil(requested)
        )
    );
}

void FreeTypeTextRenderer::drawMonoBitmap(
    OwnedMonoFramebuffer& framebuffer,
    const FT_Bitmap& bitmap,
    int x,
    int y
) {
    if (bitmap.pixel_mode != FT_PIXEL_MODE_MONO) {
        return;
    }

    for (unsigned int row = 0;
         row < bitmap.rows;
         ++row) {
        const auto* source_row =
            bitmap.buffer +
            static_cast<std::ptrdiff_t>(row) *
            bitmap.pitch;

        for (unsigned int column = 0;
             column < bitmap.width;
             ++column) {
            const auto byte =
                source_row[column / 8U];

            if ((byte &
                 (0x80U >> (column & 7U))) == 0) {
                continue;
            }

            framebuffer.setBlack(
                x + static_cast<int>(column),
                y + static_cast<int>(row)
            );
        }
    }
}

bool FreeTypeTextRenderer::renderPage(
    const PageResult& page,
    const TypographySettings& typography,
    OwnedMonoFramebuffer& framebuffer
) const {
    if (!ensureSize(typography.font_size_px)) {
        return false;
    }

    framebuffer.clearWhite();

    const int ascender =
        static_cast<int>(
            face_->size->metrics.ascender >> 6
        );

    for (const auto& line : page.lines) {
        int pen_x = line.x;
        const int baseline =
            static_cast<int>(line.y) +
            ascender;

        FT_UInt previous_glyph = 0;
        std::size_t offset = 0;
        std::uint32_t codepoint = 0;

        while (nextCodepoint(
            line.text,
            offset,
            codepoint
        )) {
            const FT_UInt glyph =
                FT_Get_Char_Index(
                    face_,
                    static_cast<FT_ULong>(
                        codepoint
                    )
                );

            if (FT_HAS_KERNING(face_) &&
                previous_glyph != 0 &&
                glyph != 0) {
                FT_Vector kerning = {};
                if (FT_Get_Kerning(
                        face_,
                        previous_glyph,
                        glyph,
                        FT_KERNING_DEFAULT,
                        &kerning
                    ) == 0) {
                    pen_x +=
                        static_cast<int>(
                            kerning.x >> 6
                        );
                }
            }

            if (FT_Load_Glyph(
                    face_,
                    glyph,
                    FT_LOAD_DEFAULT
                ) != 0 ||
                FT_Render_Glyph(
                    face_->glyph,
                    FT_RENDER_MODE_MONO
                ) != 0) {
                previous_glyph = 0;
                continue;
            }

            const auto& slot = *face_->glyph;

            drawMonoBitmap(
                framebuffer,
                slot.bitmap,
                pen_x + slot.bitmap_left,
                baseline - slot.bitmap_top
            );

            pen_x +=
                static_cast<int>(
                    slot.advance.x >> 6
                );

            previous_glyph = glyph;
        }
    }

    return true;
}

} // namespace enku::platform::esp_idf
