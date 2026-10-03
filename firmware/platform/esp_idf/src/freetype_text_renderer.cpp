#include "enku/platform/esp_idf/freetype_text_renderer.hpp"
#include "enku/core/product_info.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <utility>

#include "esp_log.h"
#include "enku/runtime/keyboard_runtime.hpp"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_FONT";

extern const std::uint8_t kEmbeddedNotoSansStart[]
    asm("_binary_NotoSans_Regular_ttf_start");
extern const std::uint8_t kEmbeddedNotoSansEnd[]
    asm("_binary_NotoSans_Regular_ttf_end");

} // namespace

FreeTypeTextRenderer::FreeTypeTextRenderer(
    OwnedMonoFramebuffer& framebuffer,
    std::string font_path
)
    : framebuffer_(framebuffer),
      font_path_(std::move(font_path)) {}

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
    if (FT_Init_FreeType(&library_) != 0) {
        return FontInitStatus::LibraryInitFailed;
    }

    bool loaded_from_sd = false;

    FILE* probe = std::fopen(
        font_path_.c_str(),
        "rb"
    );

    if (probe != nullptr) {
        std::fclose(probe);

        if (FT_New_Face(
                library_,
                font_path_.c_str(),
                0,
                &face_
            ) == 0) {
            loaded_from_sd = true;
        } else {
            ESP_LOGW(
                kTag,
                "External font invalid, falling back to embedded Noto Sans: %s",
                font_path_.c_str()
            );
        }
    }

    if (face_ == nullptr) {
        const auto embedded_size =
            static_cast<FT_Long>(
                kEmbeddedNotoSansEnd -
                kEmbeddedNotoSansStart
            );

        if (embedded_size <= 0 ||
            FT_New_Memory_Face(
                library_,
                reinterpret_cast<const FT_Byte*>(
                    kEmbeddedNotoSansStart
                ),
                embedded_size,
                0,
                &face_
            ) != 0) {
            ESP_LOGE(
                kTag,
                "Embedded Noto Sans initialization failed"
            );
            return FontInitStatus::FaceLoadFailed;
        }
    }

    if (!ensureSize(initial_size_px)) {
        return FontInitStatus::SizeFailed;
    }

    ESP_LOGI(
        kTag,
        "Loaded %s (%s) from %s",
        face_->family_name != nullptr
            ? face_->family_name
            : "unknown family",
        face_->style_name != nullptr
            ? face_->style_name
            : "unknown style",
        loaded_from_sd
            ? "SD override"
            : "embedded firmware asset"
    );

    return FontInitStatus::Ok;
}

bool FreeTypeTextRenderer::ready() const {
    return library_ != nullptr &&
           face_ != nullptr;
}

void FreeTypeTextRenderer::bindAppState(
    const AppState& app_state
) {
    app_state_ = &app_state;
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

void FreeTypeTextRenderer::setLogicalBlack(
    int x,
    int y,
    Orientation orientation
) {
    if (orientation == Orientation::Landscape) {
        framebuffer_.setBlack(x, y);
        return;
    }

    // Portrait logical surface is 480x800. Rotate clockwise into the
    // controller-native 800x480 framebuffer.
    framebuffer_.setBlack(
        static_cast<int>(framebuffer_.width()) - 1 - y,
        x
    );
}

void FreeTypeTextRenderer::drawMonoBitmap(
    const FT_Bitmap& bitmap,
    int x,
    int y,
    Orientation orientation
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

            setLogicalBlack(
                x + static_cast<int>(column),
                y + static_cast<int>(row),
                orientation
            );
        }
    }
}

bool FreeTypeTextRenderer::drawTextAt(
    std::string_view text,
    std::uint16_t size_px,
    int x,
    int baseline,
    Orientation orientation
) {
    if (!ensureSize(size_px)) {
        return false;
    }

    FT_UInt previous_glyph = 0;
    std::size_t offset = 0;
    std::uint32_t codepoint = 0;
    int pen_x = x;

    while (nextCodepoint(
        text,
        offset,
        codepoint
    )) {
        const FT_UInt glyph =
            FT_Get_Char_Index(
                face_,
                static_cast<FT_ULong>(codepoint)
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
            slot.bitmap,
            pen_x + slot.bitmap_left,
            baseline - slot.bitmap_top,
            orientation
        );

        pen_x +=
            static_cast<int>(
                slot.advance.x >> 6
            );

        previous_glyph = glyph;
    }

    return true;
}

void FreeTypeTextRenderer::drawRect(
    int x,
    int y,
    int width,
    int height,
    Orientation orientation,
    int thickness
) {
    if (width <= 0 ||
        height <= 0 ||
        thickness <= 0) {
        return;
    }

    for (int t = 0; t < thickness; ++t) {
        for (int px = x + t;
             px < x + width - t;
             ++px) {
            setLogicalBlack(px, y + t, orientation);
            setLogicalBlack(
                px,
                y + height - 1 - t,
                orientation
            );
        }

        for (int py = y + t;
             py < y + height - t;
             ++py) {
            setLogicalBlack(x + t, py, orientation);
            setLogicalBlack(
                x + width - 1 - t,
                py,
                orientation
            );
        }
    }
}

void FreeTypeTextRenderer::clearLogicalRegion(
    int x,
    int y,
    int width,
    int height,
    Orientation orientation
) {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            if (orientation == Orientation::Landscape) {
                framebuffer_.setWhite(px, py);
            } else {
                framebuffer_.setWhite(
                    static_cast<int>(framebuffer_.width()) - 1 - py,
                    px
                );
            }
        }
    }
}

void FreeTypeTextRenderer::drawWiFiStatusIcon(
    NetworkRuntimeStatus status,
    int x,
    int y,
    Orientation orientation
) {
    const auto pixel =
        [&](int px, int py) {
            setLogicalBlack(
                x + px,
                y + py,
                orientation
            );
        };

    const auto slash =
        [&]() {
            for (int i = 1; i < 14; ++i) {
                pixel(i, 14 - i);
                if (i < 13) {
                    pixel(i + 1, 14 - i);
                }
            }
        };

    if (status == NetworkRuntimeStatus::Error) {
        for (int i = 2; i < 13; ++i) {
            pixel(i, i);
            pixel(14 - i, i);
        }
        return;
    }

    // Center dot.
    for (int py = 12; py <= 14; ++py) {
        for (int px = 7; px <= 9; ++px) {
            pixel(px, py);
        }
    }

    if (status == NetworkRuntimeStatus::Connecting ||
        status == NetworkRuntimeStatus::Connected) {
        for (int i = 0; i < 5; ++i) {
            pixel(4 + i, 9 - i / 2);
            pixel(12 - i, 9 - i / 2);
        }
    }

    if (status == NetworkRuntimeStatus::Connected) {
        for (int i = 0; i < 7; ++i) {
            pixel(2 + i, 5 - i / 3);
            pixel(14 - i, 5 - i / 3);
        }
        for (int i = 0; i < 8; ++i) {
            pixel(i, 1 + i / 4);
            pixel(16 - i, 1 + i / 4);
        }
    }

    if (status == NetworkRuntimeStatus::Off ||
        status == NetworkRuntimeStatus::NoTrustedNetwork) {
        slash();
    }
}

void FreeTypeTextRenderer::drawBatteryStatusIcon(
    std::uint8_t percent,
    bool charging,
    int x,
    int y,
    Orientation orientation
) {
    constexpr int kBodyWidth = 24;
    constexpr int kBodyHeight = 12;

    drawRect(
        x,
        y,
        kBodyWidth,
        kBodyHeight,
        orientation,
        1
    );

    for (int py = 4; py < 8; ++py) {
        setLogicalBlack(
            x + kBodyWidth,
            y + py,
            orientation
        );
        setLogicalBlack(
            x + kBodyWidth + 1,
            y + py,
            orientation
        );
    }

    const int fill =
        std::max(
            0,
            std::min(
                kBodyWidth - 4,
                static_cast<int>(
                    percent
                ) * (kBodyWidth - 4) / 100
            )
        );

    for (int py = 3; py < kBodyHeight - 3; ++py) {
        for (int px = 2; px < 2 + fill; ++px) {
            setLogicalBlack(
                x + px,
                y + py,
                orientation
            );
        }
    }

    if (charging) {
        // Small lightning-like marker above the battery body.
        setLogicalBlack(x + 10, y - 2, orientation);
        setLogicalBlack(x + 9, y - 1, orientation);
        setLogicalBlack(x + 10, y - 1, orientation);
        setLogicalBlack(x + 11, y, orientation);
    }
}

bool FreeTypeTextRenderer::renderStatusBar(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    constexpr int kBarHeight = 24;
    const auto orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    clearLogicalRegion(
        0,
        0,
        logical_width,
        kBarHeight,
        orientation
    );

    drawWiFiStatusIcon(
        app_state.network.status,
        12,
        8,
        orientation
    );

    drawBatteryStatusIcon(
        app_state.power.battery_percent,
        app_state.power.charging,
        logical_width - 38,
        9,
        orientation
    );

    for (int x = 0; x < logical_width; ++x) {
        setLogicalBlack(
            x,
            kBarHeight - 1,
            orientation
        );
    }

    return true;
}


bool FreeTypeTextRenderer::drawKeyboardGrid(
    const KeyboardState& keyboard,
    Orientation orientation,
    int top
) {
    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const auto key_count =
        keyboardKeyCount(
            keyboard.mode
        );
    const auto character_count =
        keyboardCharacterKeyCount(
            keyboard.mode
        );

    const int key_height =
        orientation == Orientation::Portrait
            ? 42
            : 34;
    const int gap = 6;

    const int character_columns =
        orientation == Orientation::Portrait
            ? 8
            : 10;
    const int character_width =
        (logical_width - 48 -
         gap * (character_columns - 1)) /
        character_columns;

    for (std::uint16_t i = 0;
         i < character_count;
         ++i) {
        const int row =
            static_cast<int>(i) /
            character_columns;
        const int column =
            static_cast<int>(i) %
            character_columns;

        const int x =
            24 +
            column *
                (character_width + gap);
        const int y =
            top +
            row *
                (key_height + gap);

        drawRect(
            x,
            y,
            character_width,
            key_height,
            orientation,
            keyboard.focus_index == i
                ? 2
                : 1
        );

        const auto key =
            keyboardKeyLabel(
                keyboard,
                i
            );

        if (!drawTextAt(
                key,
                14,
                x + 8,
                y + key_height - 11,
                orientation
            )) {
            return false;
        }
    }

    const int character_rows =
        (static_cast<int>(character_count) +
         character_columns - 1) /
        character_columns;
    const int special_y =
        top +
        character_rows *
            (key_height + gap);
    const int special_width =
        (logical_width - 48 - gap * 4) /
        5;

    for (std::uint16_t i = character_count;
         i < key_count;
         ++i) {
        const int column =
            static_cast<int>(
                i - character_count
            );
        const int x =
            24 +
            column *
                (special_width + gap);

        drawRect(
            x,
            special_y,
            special_width,
            key_height,
            orientation,
            keyboard.focus_index == i
                ? 2
                : 1
        );

        const auto key =
            keyboardKeyLabel(
                keyboard,
                i
            );

        if (!drawTextAt(
                key,
                12,
                x + 6,
                special_y +
                    key_height - 11,
                orientation
            )) {
            return false;
        }
    }

    return true;
}

bool FreeTypeTextRenderer::renderPage(
    const PageResult& page,
    const TypographySettings& typography,
    Orientation orientation
) {
    if (!ensureSize(typography.font_size_px)) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const int ascender =
        static_cast<int>(
            face_->size->metrics.ascender >> 6
        );

    const int reader_line_height =
        static_cast<int>(
            lineHeightPx(typography)
        );

    for (const auto& line : page.lines) {
        int pen_x = line.x;
        const int baseline =
            static_cast<int>(line.y) +
            ascender;

        if (line.kind == PageLineKind::Quote) {
            drawRect(
                std::max(
                    0,
                    static_cast<int>(line.x) - 12
                ),
                static_cast<int>(line.y) + 1,
                2,
                std::max(
                    2,
                    reader_line_height - 3
                ),
                orientation,
                1
            );
        } else if (
            line.kind == PageLineKind::ListItem
        ) {
            drawRect(
                std::max(
                    0,
                    static_cast<int>(line.x) - 13
                ),
                baseline - 5,
                4,
                4,
                orientation,
                1
            );
        }

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
                slot.bitmap,
                pen_x + slot.bitmap_left,
                baseline - slot.bitmap_top,
                orientation
            );

            pen_x +=
                static_cast<int>(
                    slot.advance.x >> 6
                );

            previous_glyph = glyph;
        }

        if (line.kind == PageLineKind::Heading) {
            const int heading_width =
                static_cast<int>(
                    measureWidthPx(
                        line.text,
                        typography
                    )
                );

            if (heading_width > 0) {
                drawRect(
                    static_cast<int>(line.x),
                    baseline + 2,
                    heading_width,
                    1,
                    orientation,
                    1
                );
            }
        }

        if (app_state_ != nullptr &&
            app_state_->search_highlight.position.has_value() &&
            !app_state_->search_highlight.query.empty()) {
            const auto& highlight =
                *app_state_->search_highlight.position;

            if (highlight.book_id == line.position.book_id &&
                highlight.section_id == line.position.section_id &&
                highlight.text_offset >= line.position.text_offset) {
                const auto local =
                    highlight.text_offset -
                    line.position.text_offset;

                if (local <= line.text.size() &&
                    local + app_state_->search_highlight.query.size() <=
                        line.text.size()) {
                    const auto prefix =
                        line.text.substr(
                            0,
                            static_cast<std::size_t>(local)
                        );

                    const int highlight_x =
                        static_cast<int>(line.x) +
                        static_cast<int>(
                            measureWidthPx(
                                prefix,
                                typography
                            )
                        );

                    const int highlight_width =
                        static_cast<int>(
                            measureWidthPx(
                                app_state_->search_highlight.query,
                                typography
                            )
                        );

                    if (highlight_width > 0) {
                        drawRect(
                            highlight_x,
                            baseline + 2,
                            highlight_width,
                            2,
                            orientation,
                            1
                        );
                    }
                }
            }
        }
    }

    return true;
}

bool FreeTypeTextRenderer::renderLibrary(
    const AppState& app_state,
    const LibraryPage& page
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const Orientation orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const bool search_mode =
        app_state.library.mode ==
        LibraryQueryMode::Search;

    const bool storage_warning =
        !search_mode &&
        app_state.storage.removable !=
            RemovableStorageStatus::Ready;

    if (!drawTextAt(
            search_mode
                ? "SEARCH LIBRARY"
                : "LIBRARY",
            28,
            32,
            48,
            orientation
        )) {
        return false;
    }

    char count_text[64] = {};
    const auto page_end =
        page.offset +
        static_cast<std::uint32_t>(
            page.items.size()
        );

    if (page.total_matches == 0U) {
        std::snprintf(
            count_text,
            sizeof(count_text),
            "0 BOOKS"
        );
    } else {
        std::snprintf(
            count_text,
            sizeof(count_text),
            "%lu-%lu OF %lu",
            static_cast<unsigned long>(
                page.offset + 1U
            ),
            static_cast<unsigned long>(
                page_end
            ),
            static_cast<unsigned long>(
                page.total_matches
            )
        );
    }

    if (!drawTextAt(
            count_text,
            14,
            logical_width - 150,
            42,
            orientation
        )) {
        return false;
    }

    int start_y = 78;

    if (storage_warning) {
        drawRect(
            24,
            70,
            logical_width - 48,
            42,
            orientation,
            2
        );

        const char* storage_message =
            app_state.storage.removable ==
                RemovableStorageStatus::SetupError
                ? "SD CARD ERROR"
                : "SD CARD UNAVAILABLE";

        if (!drawTextAt(
                storage_message,
                14,
                38,
                96,
                orientation
            )) {
            return false;
        }

        start_y = 126;
    }

    if (search_mode) {
        drawRect(
            24,
            72,
            logical_width - 48,
            50,
            orientation,
            2
        );

        const std::string query =
            app_state.library.search_text.empty()
                ? "SEARCH TITLE OR AUTHOR"
                : app_state.library.search_text;

        if (!drawTextAt(
                query,
                17,
                40,
                104,
                orientation
            )) {
            return false;
        }

        if (app_state.keyboard.open) {
            if (!drawTextAt(
                    "KEYBOARD",
                    13,
                    30,
                    152,
                    orientation
                )) {
                return false;
            }

            if (!drawKeyboardGrid(
                    app_state.keyboard,
                    orientation,
                    orientation == Orientation::Portrait
                        ? 184
                        : 174
                )) {
                return false;
            }

            return drawTextAt(
                "UP/DOWN KEY  FUNCTION SELECT  BACK CANCEL",
                12,
                30,
                orientation == Orientation::Portrait
                    ? 760
                    : 448,
                orientation
            );
        }

        start_y = 144;
    }

    constexpr int kRowHeight = 64;
    constexpr int kLeft = 24;
    const int kWidth = logical_width - 48;
    const std::size_t kMaxVisible =
        search_mode
            ? (orientation == Orientation::Portrait
                ? 8U
                : 5U)
            : (orientation == Orientation::Portrait
                ? 10U
                : (storage_warning ? 5U : 6U));

    std::size_t focused_index = 0U;
    bool has_focus = false;

    if (app_state.library.focused_book.has_value()) {
        for (std::size_t i = 0;
             i < page.items.size();
             ++i) {
            if (page.items[i].book_id ==
                *app_state.library.focused_book) {
                focused_index = i;
                has_focus = true;
                break;
            }
        }
    }

    const std::size_t visible_start =
        has_focus
            ? (focused_index / kMaxVisible) *
                kMaxVisible
            : 0U;

    const auto visible =
        std::min<std::size_t>(
            page.items.size() - visible_start,
            kMaxVisible
        );

    for (std::size_t row = 0;
         row < visible;
         ++row) {
        const auto index =
            visible_start + row;
        const auto& book = page.items[index];
        const int top =
            start_y +
            static_cast<int>(row) *
                kRowHeight;

        const bool focused =
            has_focus &&
            focused_index == index;

        if (focused) {
            drawRect(
                kLeft,
                top,
                kWidth,
                kRowHeight - 6,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                book.metadata.title,
                18,
                kLeft + 14,
                top + 25,
                orientation
            )) {
            return false;
        }

        const std::string meta =
            book.metadata.author_display.empty()
                ? "Unknown author"
                : book.metadata.author_display;

        if (!drawTextAt(
                meta,
                13,
                kLeft + 14,
                top + 46,
                orientation
            )) {
            return false;
        }

        char progress[16] = {};
        std::snprintf(
            progress,
            sizeof(progress),
            "%u%%",
            static_cast<unsigned>(
                std::min(
                    100.0F,
                    std::max(
                        0.0F,
                        book.progress * 100.0F
                    )
                )
            )
        );

        if (!drawTextAt(
                progress,
                13,
                kLeft + kWidth - 62,
                top + 35,
                orientation
            )) {
            return false;
        }
    }

    if (page.items.empty()) {
        if (!drawTextAt(
                search_mode
                    ? "NO RESULTS"
                    : "NO BOOKS",
                18,
                32,
                start_y + 42,
                orientation
            )) {
            return false;
        }

        if (!search_mode) {
            const char* empty_hint =
                app_state.storage.removable ==
                    RemovableStorageStatus::Ready
                    ? "IMPORT A TXT BOOK TO START"
                    : "INSERT OR REPAIR SD CARD";

            if (!drawTextAt(
                    empty_hint,
                    14,
                    32,
                    start_y + 72,
                    orientation
                )) {
                return false;
            }
        }
    }

    if (search_mode) {
        return drawTextAt(
            "FUNCTION EDIT  BACK CLEAR SEARCH",
            12,
            30,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    return true;
}

} // namespace enku::platform::esp_idf

namespace enku::platform::esp_idf {

bool FreeTypeTextRenderer::renderWiFiSettings(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const auto& state =
        app_state.wifi_settings;

    if (!drawTextAt(
            "WI-FI",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    const auto policyLabel =
        [](WiFiPolicy policy) -> const char* {
            switch (policy) {
                case WiFiPolicy::Off:
                    return "OFF";
                case WiFiPolicy::Manual:
                    return "MANUAL";
                case WiFiPolicy::AutoConnectTrusted:
                    return "AUTO-CONNECT TRUSTED";
            }
            return "UNKNOWN";
        };

    if (state.forget_confirm) {
        if (!drawTextAt(
                "FORGET TRUSTED NETWORK?",
                20,
                28,
                118,
                orientation
            )) {
            return false;
        }

        const int gap = 16;
        const int button_width =
            (logical_width - 64 - gap) / 2;
        const int top =
            orientation == Orientation::Portrait
                ? 240
                : 210;

        const bool forget_selected =
            state.confirm_forget;

        drawRect(
            24,
            top,
            button_width,
            54,
            orientation,
            forget_selected ? 1 : 2
        );

        drawRect(
            24 + button_width + gap,
            top,
            button_width,
            54,
            orientation,
            forget_selected ? 2 : 1
        );

        if (!drawTextAt(
                "CANCEL",
                17,
                40,
                top + 34,
                orientation
            ) ||
            !drawTextAt(
                "FORGET",
                17,
                40 + button_width + gap,
                top + 34,
                orientation
            )) {
            return false;
        }

        return drawTextAt(
            "UP/DOWN CHOOSE  FUNCTION CONFIRM  BACK CANCEL",
            12,
            28,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (app_state.keyboard.open) {
        if (!drawTextAt(
                state.pending_ssid,
                18,
                28,
                102,
                orientation
            )) {
            return false;
        }

        std::string masked(
            state.pending_password.size(),
            '*'
        );

        if (masked.empty()) {
            masked = "ENTER PASSWORD";
        }

        drawRect(
            24,
            126,
            logical_width - 48,
            52,
            orientation,
            2
        );

        if (!drawTextAt(
                masked,
                16,
                38,
                159,
                orientation
            ) ||
            !drawKeyboardGrid(
                app_state.keyboard,
                orientation,
                orientation == Orientation::Portrait
                    ? 206
                    : 190
            )) {
            return false;
        }

        return drawTextAt(
            "UP/DOWN KEY  FUNCTION SELECT  BACK NETWORKS",
            12,
            28,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (state.selecting_network) {
        if (!drawTextAt(
                "SELECT NETWORK",
                17,
                28,
                92,
                orientation
            )) {
            return false;
        }

        const std::size_t visible_count =
            orientation == Orientation::Portrait
                ? 8U
                : 5U;

        const std::size_t focus =
            std::min<std::size_t>(
                state.network_focus,
                state.scan_results.empty()
                    ? 0U
                    : state.scan_results.size() - 1U
            );

        const std::size_t window_start =
            state.scan_results.empty()
                ? 0U
                : (focus / visible_count) *
                    visible_count;

        const auto visible =
            std::min<std::size_t>(
                visible_count,
                state.scan_results.size() -
                    window_start
            );

        const int start_y =
            orientation == Orientation::Portrait
                ? 116
                : 108;
        const int row_height =
            orientation == Orientation::Portrait
                ? 66
                : 56;

        for (std::size_t row = 0;
             row < visible;
             ++row) {
            const auto index =
                window_start + row;
            const auto& network =
                state.scan_results[index];
            const int top =
                start_y +
                static_cast<int>(row) *
                    row_height;

            if (index == focus) {
                drawRect(
                    20,
                    top,
                    logical_width - 40,
                    row_height - 6,
                    orientation,
                    2
                );
            }

            if (!drawTextAt(
                    network.ssid,
                    16,
                    34,
                    top + 28,
                    orientation
                )) {
                return false;
            }

            char meta[48] = {};
            std::snprintf(
                meta,
                sizeof(meta),
                "%s  %ld dBm",
                network.secured
                    ? "SECURED"
                    : "OPEN",
                static_cast<long>(
                    network.rssi
                )
            );

            if (!drawTextAt(
                    meta,
                    11,
                    34,
                    top + 48,
                    orientation
                )) {
                return false;
            }
        }

        if (!state.status_message.empty()) {
            if (!drawTextAt(
                    state.status_message,
                    12,
                    28,
                    orientation == Orientation::Portrait
                        ? 700
                        : 408,
                    orientation
                )) {
                return false;
            }
        }

        return drawTextAt(
            "UP/DOWN NETWORK  FUNCTION CONNECT  BACK WI-FI",
            12,
            28,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (!drawTextAt(
            "POLICY",
            13,
            28,
            100,
            orientation
        )) {
        return false;
    }

    if (state.focus ==
        WiFiSettingsFocus::Policy) {
        drawRect(
            20,
            114,
            logical_width - 40,
            58,
            orientation,
            state.editing_policy ? 3 : 2
        );
    }

    if (!drawTextAt(
            policyLabel(
                state.editing_policy
                    ? state.selected_policy
                    : app_state.wifi_policy
            ),
            16,
            36,
            150,
            orientation
        )) {
        return false;
    }

    const int scan_top = 192;

    if (state.focus ==
        WiFiSettingsFocus::ScanNetworks) {
        drawRect(
            20,
            scan_top,
            logical_width - 40,
            54,
            orientation,
            2
        );
    }

    if (!drawTextAt(
            "SCAN & CONNECT",
            16,
            36,
            scan_top + 34,
            orientation
        )) {
        return false;
    }

    const auto connectionLabel =
        [](NetworkRuntimeStatus status) -> const char* {
            switch (status) {
                case NetworkRuntimeStatus::Off:
                    return "OFF";
                case NetworkRuntimeStatus::Idle:
                    return "IDLE";
                case NetworkRuntimeStatus::Connecting:
                    return "CONNECTING";
                case NetworkRuntimeStatus::Connected:
                    return "CONNECTED";
                case NetworkRuntimeStatus::NoTrustedNetwork:
                    return "NO TRUSTED NETWORK";
                case NetworkRuntimeStatus::Error:
                    return "ERROR";
            }
            return "UNKNOWN";
        };

    const char* connection =
        connectionLabel(
            app_state.network.status
        );

    const int status_label_y =
        orientation == Orientation::Portrait
            ? 280
            : 252;
    const int status_value_y =
        orientation == Orientation::Portrait
            ? 308
            : 276;
    const int trusted_y =
        orientation == Orientation::Portrait
            ? 340
            : 304;
    const int address_y =
        orientation == Orientation::Portrait
            ? 366
            : 326;

    if (!drawTextAt(
            "STATUS",
            12,
            28,
            status_label_y,
            orientation
        ) ||
        !drawTextAt(
            connection,
            15,
            36,
            status_value_y,
            orientation
        )) {
        return false;
    }

    const std::string trusted =
        app_state.network.ssid.empty()
            ? "TRUSTED: NONE"
            : "TRUSTED: " +
                app_state.network.ssid;

    if (!drawTextAt(
            trusted,
            13,
            28,
            trusted_y,
            orientation
        )) {
        return false;
    }

    const std::string web_address =
        app_state.network.status ==
                NetworkRuntimeStatus::Connected
            ? "WEB: http://enku.local"
            : "WEB: NOT AVAILABLE";

    if (!drawTextAt(
            web_address,
            13,
            28,
            address_y,
            orientation
        )) {
        return false;
    }

    if (!app_state.network.address.empty()) {
        const std::string ip_fallback =
            "IP: http://" +
            app_state.network.address;

        if (!drawTextAt(
                ip_fallback,
                11,
                28,
                address_y + 22,
                orientation
            )) {
            return false;
        }
    }

    const int forget_top =
        orientation == Orientation::Portrait
            ? 410
            : 378;

    if (state.focus ==
        WiFiSettingsFocus::ForgetTrusted) {
        drawRect(
            20,
            forget_top,
            logical_width - 40,
            54,
            orientation,
            2
        );
    }

    if (!drawTextAt(
            "FORGET TRUSTED NETWORK",
            15,
            36,
            forget_top + 34,
            orientation
        )) {
        return false;
    }

    if (!state.status_message.empty()) {
        if (!drawTextAt(
                state.status_message,
                12,
                28,
                forget_top + 82,
                orientation
            )) {
            return false;
        }
    }

    return drawTextAt(
        state.editing_policy
            ? "UP/DOWN CHANGE  FUNCTION APPLY  BACK CANCEL"
            : "UP/DOWN MOVE  FUNCTION SELECT  BACK SETTINGS",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderPowerOffConfirm(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "POWER OFF?",
            28,
            28,
            72,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "SAVE STATE AND REQUEST DEVICE POWER OFF.",
            14,
            28,
            118,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "WAKE BEHAVIOR REQUIRES HARDWARE VALIDATION.",
            12,
            28,
            146,
            orientation
        )) {
        return false;
    }

    const int gap = 16;
    const int button_width =
        (logical_width - 64 - gap) / 2;
    const int top =
        orientation == Orientation::Portrait
            ? 250
            : 220;

    const bool power_selected =
        app_state.power_off_confirm.focus ==
        PowerOffConfirmFocus::PowerOff;

    drawRect(
        24,
        top,
        button_width,
        54,
        orientation,
        power_selected ? 1 : 2
    );

    drawRect(
        24 + button_width + gap,
        top,
        button_width,
        54,
        orientation,
        power_selected ? 2 : 1
    );

    if (!drawTextAt(
            "CANCEL",
            17,
            40,
            top + 34,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "POWER OFF",
            17,
            40 + button_width + gap,
            top + 34,
            orientation
        )) {
        return false;
    }

    return drawTextAt(
        "UP/DOWN CHOOSE  FUNCTION CONFIRM  BACK CANCEL",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderAboutDevice(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "ABOUT",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            kProductName,
            30,
            28,
            118,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            kProductDescription,
            15,
            28,
            148,
            orientation
        )) {
        return false;
    }

    const struct {
        const char* label;
        std::string_view value;
    } fields[] = {
        {"FIRMWARE", kFirmwareVersion},
        {"HARDWARE", kHardwareTarget},
        {"DISPLAY", kDisplayDescription},
        {"INPUT", kInputDescription},
    };

    const int start_y =
        orientation == Orientation::Portrait
            ? 222
            : 112;

    const int row_height =
        orientation == Orientation::Portrait
            ? 74
            : 68;

    const int value_x =
        orientation == Orientation::Portrait
            ? 28
            : 420;

    for (int i = 0; i < 4; ++i) {
        const int top =
            start_y + i * row_height;

        if (!drawTextAt(
                fields[i].label,
                11,
                value_x,
                top,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                fields[i].value,
                15,
                value_x,
                top + 24,
                orientation
            )) {
            return false;
        }
    }

    return drawTextAt(
        "FUNCTION OR BACK  RETURN TO SETTINGS",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderLocaleSettings(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "LANGUAGE",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    struct Entry {
        const char* code;
        const char* name;
    };

    constexpr Entry entries[] = {
        {"EN", "English"},
        {"RU", "Русский"},
        {"PL", "Polski"},
        {"DE", "Deutsch"},
        {"FR", "Français"},
        {"ES", "Español"},
        {"IT", "Italiano"},
    };

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const std::uint8_t visible =
        orientation == Orientation::Portrait
            ? 7U
            : 5U;

    const int start_y =
        orientation == Orientation::Portrait
            ? 96
            : 76;

    const int row_height =
        orientation == Orientation::Portrait
            ? 82
            : 64;

    const auto start =
        app_state.locale_settings.window_start;

    const auto end =
        static_cast<std::uint8_t>(
            std::min<int>(
                7,
                static_cast<int>(start) +
                static_cast<int>(visible)
            )
        );

    for (std::uint8_t i = start;
         i < end;
         ++i) {
        const int row =
            static_cast<int>(i - start);

        const int top =
            start_y + row * row_height;

        if (app_state.locale_settings.focus_index == i) {
            drawRect(
                20,
                top,
                logical_width - 40,
                row_height - 8,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                entries[i].code,
                17,
                36,
                top + 30,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                entries[i].name,
                14,
                92,
                top + 30,
                orientation
            )) {
            return false;
        }

        const LocaleId locale_map[] = {
            LocaleId::En,
            LocaleId::Ru,
            LocaleId::Pl,
            LocaleId::De,
            LocaleId::Fr,
            LocaleId::Es,
            LocaleId::It,
        };

        if (app_state.ui_locale == locale_map[i]) {
            if (!drawTextAt(
                    "ACTIVE",
                    11,
                    logical_width - 88,
                    top + 30,
                    orientation
                )) {
                return false;
            }
        }
    }

    return drawTextAt(
        "UP/DOWN MOVE  FUNCTION APPLY  BACK SETTINGS",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderDisplaySettings(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "DISPLAY",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "ORIENTATION",
            16,
            28,
            112,
            orientation
        )) {
        return false;
    }

    const int gap = 12;
    const int button_width =
        (logical_width - 56 - gap) / 2;
    const int top = 132;

    const bool selected_portrait =
        app_state.display_settings.selected ==
        Orientation::Portrait;

    drawRect(
        24,
        top,
        button_width,
        48,
        orientation,
        selected_portrait ? 3 : 1
    );

    drawRect(
        24 + button_width + gap,
        top,
        button_width,
        48,
        orientation,
        selected_portrait ? 1 : 3
    );

    if (!drawTextAt(
            "PORTRAIT",
            15,
            42,
            top + 31,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "LANDSCAPE",
            15,
            42 + button_width + gap,
            top + 31,
            orientation
        )) {
        return false;
    }

    std::string applied =
        "APPLIED: ";

    applied +=
        app_state.orientation ==
                Orientation::Portrait
            ? "PORTRAIT"
            : "LANDSCAPE";

    if (!drawTextAt(
            applied,
            12,
            28,
            206,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "REFRESH",
            16,
            28,
            264,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "HARDWARE VALIDATION PENDING",
            14,
            28,
            296,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "BALANCED / CLEAN AND MANUAL REFRESH ARE",
            11,
            28,
            326,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "NOT ENABLED UNTIL REAL PANEL GHOSTING TESTS.",
            11,
            28,
            346,
            orientation
        )) {
        return false;
    }

    return drawTextAt(
        "UP/DOWN SELECT  FUNCTION APPLY  BACK SETTINGS",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderReadingSettings(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "READING SETTINGS",
            25,
            28,
            50,
            orientation
        )) {
        return false;
    }

    const char* preset = "STANDARD";

    switch (app_state.typography.preset) {
        case ReadingPreset::Spacious:
            preset = "SPACIOUS";
            break;
        case ReadingPreset::Comfortable:
            preset = "COMFORTABLE";
            break;
        case ReadingPreset::Standard:
            preset = "STANDARD";
            break;
        case ReadingPreset::Compact:
            preset = "COMPACT";
            break;
        case ReadingPreset::Dense:
            preset = "DENSE";
            break;
        case ReadingPreset::Custom:
            preset = "CUSTOM";
            break;
    }

    char font_size[32] = {};
    std::snprintf(
        font_size,
        sizeof(font_size),
        "%u PX",
        static_cast<unsigned>(
            app_state.typography.font_size_px
        )
    );

    char line_spacing[32] = {};
    std::snprintf(
        line_spacing,
        sizeof(line_spacing),
        "%.2f",
        static_cast<double>(
            app_state.typography.line_spacing
        )
    );

    char margins[32] = {};
    std::snprintf(
        margins,
        sizeof(margins),
        "%u PX",
        static_cast<unsigned>(
            app_state.typography.margin_px
        )
    );

    const char* labels[] = {
        "PRESET",
        "FONT SIZE",
        "LINE SPACING",
        "MARGINS",
    };

    const char* values[] = {
        preset,
        font_size,
        line_spacing,
        margins,
    };

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const int start_y =
        orientation == Orientation::Portrait
            ? 120
            : 92;

    const int row_height =
        orientation == Orientation::Portrait
            ? 92
            : 66;

    for (int i = 0; i < 4; ++i) {
        const int top =
            start_y + i * row_height;

        const bool focused =
            static_cast<int>(
                app_state.reading_settings.focus
            ) == i;

        if (focused) {
            drawRect(
                20,
                top,
                logical_width - 40,
                row_height - 10,
                orientation,
                app_state.reading_settings.editing
                    ? 3
                    : 2
            );
        }

        if (!drawTextAt(
                labels[i],
                orientation == Orientation::Portrait
                    ? 16
                    : 14,
                36,
                top + (
                    orientation == Orientation::Portrait
                        ? 31
                        : 25
                ),
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                values[i],
                orientation == Orientation::Portrait
                    ? 13
                    : 12,
                36,
                top + (
                    orientation == Orientation::Portrait
                        ? 58
                        : 47
                ),
                orientation
            )) {
            return false;
        }
    }

    if (app_state.reading_settings.editing) {
        return drawTextAt(
            "UP/DOWN CHANGE  FUNCTION SAVE  BACK CANCEL",
            12,
            28,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    return drawTextAt(
        "UP/DOWN MOVE  FUNCTION EDIT  BACK SETTINGS",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderSettingsScreen(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "SETTINGS",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    const char* preset = "STANDARD";

    switch (app_state.typography.preset) {
        case ReadingPreset::Spacious:
            preset = "SPACIOUS";
            break;
        case ReadingPreset::Comfortable:
            preset = "COMFORTABLE";
            break;
        case ReadingPreset::Standard:
            preset = "STANDARD";
            break;
        case ReadingPreset::Compact:
            preset = "COMPACT";
            break;
        case ReadingPreset::Dense:
            preset = "DENSE";
            break;
        case ReadingPreset::Custom:
            preset = "CUSTOM";
            break;
    }

    const char* orient =
        app_state.orientation ==
                Orientation::Portrait
            ? "PORTRAIT"
            : "LANDSCAPE";

    std::string wifi =
        app_state.network.connected
            ? (
                app_state.network.ssid.empty()
                    ? "CONNECTED"
                    : app_state.network.ssid
              )
            : (
                app_state.wifi_policy ==
                        WiFiPolicy::AutoConnectTrusted
                    ? "AUTO / OFFLINE"
                    : "OFF"
              );

    const char* language =
        app_state.ui_locale == LocaleId::En
            ? "ENGLISH"
            : "OTHER";

    const char* labels[] = {
        "READING",
        "DISPLAY",
        "WI-FI",
        "LANGUAGE",
        "STORAGE",
        "SLEEP",
        "ABOUT",
        "POWER OFF",
    };

    std::string values[8];
    values[0] = preset;
    values[1] = orient;
    values[2] = wifi;
    values[3] = language;
    values[4] =
        std::to_string(
            app_state.library.total_matches
        ) + " BOOKS";
    values[5] = "MANUAL AVAILABLE";
    values[6] = "ENKU";
    values[7] = "";

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const int start_y =
        orientation == Orientation::Portrait
            ? 92
            : 66;

    const int row_height =
        orientation == Orientation::Portrait
            ? 68
            : 46;

    for (int i = 0; i < 8; ++i) {
        const int top =
            start_y + i * row_height;

        if (static_cast<int>(
                app_state.settings_nav.focus
            ) == i) {
            drawRect(
                16,
                top,
                logical_width - 32,
                row_height - 6,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                labels[i],
                orientation == Orientation::Portrait
                    ? 16
                    : 14,
                30,
                top + (
                    orientation == Orientation::Portrait
                        ? 27
                        : 21
                ),
                orientation
            )) {
            return false;
        }

        if (!values[i].empty()) {
            if (!drawTextAt(
                    values[i],
                    orientation == Orientation::Portrait
                        ? 11
                        : 10,
                    30,
                    top + (
                        orientation == Orientation::Portrait
                            ? 47
                            : 36
                    ),
                    orientation
                )) {
                return false;
            }
        }
    }

    return drawTextAt(
        "UP/DOWN MOVE  FUNCTION OPEN  BACK LIBRARY",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderAboutBook(
    const AppState& app_state,
    const BookRecord& book
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    if (!drawTextAt(
            "ABOUT BOOK",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            book.metadata.title.empty()
                ? "UNTITLED"
                : book.metadata.title,
            24,
            30,
            104,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            book.metadata.author_display.empty()
                ? "UNKNOWN AUTHOR"
                : book.metadata.author_display,
            15,
            30,
            136,
            orientation
        )) {
        return false;
    }

    const char* state = "NEW";

    if (book.reading_state ==
        ReadingState::Reading) {
        state = "READING";
    } else if (
        book.reading_state ==
        ReadingState::Finished) {
        state = "FINISHED";
    }

    char progress[64] = {};
    std::snprintf(
        progress,
        sizeof(progress),
        "%s  %u%%",
        state,
        static_cast<unsigned>(
            std::min(
                100.0F,
                std::max(
                    0.0F,
                    book.progress * 100.0F
                )
            )
        )
    );

    if (!drawTextAt(
            progress,
            14,
            30,
            176,
            orientation
        )) {
        return false;
    }

    const std::string language =
        book.metadata.language.has_value()
            ? "LANGUAGE: " + *book.metadata.language
            : "LANGUAGE: UNKNOWN";

    if (!drawTextAt(
            language,
            13,
            30,
            206,
            orientation
        )) {
        return false;
    }

    if (book.metadata.series_name.has_value() &&
        !book.metadata.series_name->empty()) {
        std::string series =
            "SERIES: " + *book.metadata.series_name;

        if (book.metadata.series_index.has_value()) {
            series += " #" +
                std::to_string(
                    *book.metadata.series_index
                );
        }

        if (!drawTextAt(
                series,
                13,
                30,
                236,
                orientation
            )) {
            return false;
        }
    }

    std::string description =
        book.metadata.description.has_value() &&
        !book.metadata.description->empty()
            ? *book.metadata.description
            : "NO DESCRIPTION";

    const std::size_t limit =
        orientation == Orientation::Portrait
            ? 160U
            : 240U;

    if (description.size() > limit) {
        description.resize(limit);
        description += "...";
    }

    if (!drawTextAt(
            description,
            13,
            30,
            286,
            orientation
        )) {
        return false;
    }

    return drawTextAt(
        "FUNCTION OR BACK  RETURN TO READING",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderContentsBookmarks(
    const AppState& app_state,
    const std::vector<ContentsEntry>& contents,
    const std::vector<BookmarkRecord>& bookmarks
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const auto& state =
        app_state.contents_bookmarks;

    if (!drawTextAt(
            "CONTENTS & BOOKMARKS",
            24,
            28,
            48,
            orientation
        )) {
        return false;
    }

    const int tab_top = 72;
    const int tab_gap = 12;
    const int tab_width =
        (logical_width - 56 - tab_gap) / 2;

    const bool contents_active =
        state.tab == ContentsBookmarksTab::Contents;

    drawRect(
        24,
        tab_top,
        tab_width,
        48,
        orientation,
        state.focus ==
                ContentsBookmarksFocus::ContentsTab
            ? 2
            : 1
    );

    drawRect(
        24 + tab_width + tab_gap,
        tab_top,
        tab_width,
        48,
        orientation,
        state.focus ==
                ContentsBookmarksFocus::BookmarksTab
            ? 2
            : 1
    );

    if (!drawTextAt(
            contents_active
                ? "CONTENTS *"
                : "CONTENTS",
            15,
            38,
            tab_top + 30,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            contents_active
                ? "BOOKMARKS"
                : "BOOKMARKS *",
            15,
            38 + tab_width + tab_gap,
            tab_top + 30,
            orientation
        )) {
        return false;
    }

    const std::size_t count =
        contents_active
            ? contents.size()
            : bookmarks.size();

    const std::uint32_t visible =
        orientation == Orientation::Portrait
            ? 8U
            : 5U;

    const auto start =
        std::min<std::size_t>(
            state.window_start,
            count
        );

    const auto end =
        std::min<std::size_t>(
            count,
            start + visible
        );

    const int list_top = 142;
    const int row_height =
        orientation == Orientation::Portrait
            ? 62
            : 54;

    if (count == 0U) {
        if (!drawTextAt(
                contents_active
                    ? "NO CONTENTS"
                    : "NO BOOKMARKS",
                18,
                34,
                list_top + 40,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                contents_active
                    ? "THIS BOOK HAS NO STRUCTURED CONTENTS."
                    : "ADD A BOOKMARK FROM THE READER MENU.",
                13,
                34,
                list_top + 70,
                orientation
            )) {
            return false;
        }
    } else {
        for (std::size_t i = start;
             i < end;
             ++i) {
            const int row =
                static_cast<int>(i - start);

            const int top =
                list_top + row * row_height;

            const bool focused =
                state.focus ==
                    ContentsBookmarksFocus::Item &&
                state.item_index == i;

            if (focused) {
                drawRect(
                    24,
                    top,
                    logical_width - 48,
                    row_height - 6,
                    orientation,
                    2
                );
            }

            std::string label;

            if (contents_active) {
                label = contents[i].label;
            } else {
                label = bookmarks[i].label.empty()
                    ? "Bookmark"
                    : bookmarks[i].label;
            }

            if (!drawTextAt(
                    label,
                    16,
                    40,
                    top + 30,
                    orientation
                )) {
                return false;
            }

            if (!contents_active) {
                char offset[40] = {};
                std::snprintf(
                    offset,
                    sizeof(offset),
                    "POSITION %llu",
                    static_cast<unsigned long long>(
                        bookmarks[i].
                            position.text_offset
                    )
                );

                if (!drawTextAt(
                        offset,
                        11,
                        40,
                        top + 47,
                        orientation
                    )) {
                    return false;
                }
            }
        }
    }

    return drawTextAt(
        "UP/DOWN MOVE  FUNCTION SELECT  BACK READING",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderBookFinished(
    const AppState& app_state,
    const BookRecord& book
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;
    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "BOOK FINISHED",
            28,
            30,
            58,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            book.metadata.title.empty()
                ? "UNTITLED"
                : book.metadata.title,
            24,
            30,
            112,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            "100% COMPLETE",
            16,
            30,
            150,
            orientation
        )) {
        return false;
    }

    if (app_state.book_finished.restart_confirm) {
        if (!drawTextAt(
                "READ AGAIN FROM THE BEGINNING?",
                18,
                30,
                212,
                orientation
            )) {
            return false;
        }

        const int gap = 16;
        const int button_width =
            (logical_width - 64 - gap) / 2;
        const int top =
            orientation == Orientation::Portrait
                ? 280
                : 240;

        drawRect(
            24,
            top,
            button_width,
            54,
            orientation,
            app_state.book_finished.confirm_restart
                ? 1
                : 2
        );
        drawRect(
            24 + button_width + gap,
            top,
            button_width,
            54,
            orientation,
            app_state.book_finished.confirm_restart
                ? 2
                : 1
        );

        if (!drawTextAt(
                "CANCEL",
                17,
                40,
                top + 34,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                "RESTART",
                17,
                40 + button_width + gap,
                top + 34,
                orientation
            )) {
            return false;
        }

        return drawTextAt(
            "UP/DOWN CHOOSE  FUNCTION CONFIRM  BACK CANCEL",
            12,
            30,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    const struct {
        BookFinishedFocus focus;
        const char* label;
    } actions[] = {
        {
            BookFinishedFocus::BackToLibrary,
            "BACK TO LIBRARY",
        },
        {
            BookFinishedFocus::ReadAgain,
            "READ AGAIN",
        },
    };

    const int top =
        orientation == Orientation::Portrait
            ? 250
            : 220;

    for (int i = 0; i < 2; ++i) {
        const int y = top + i * 64;

        if (app_state.book_finished.focus ==
            actions[i].focus) {
            drawRect(
                24,
                y,
                logical_width - 48,
                54,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                actions[i].label,
                18,
                40,
                y + 34,
                orientation
            )) {
            return false;
        }
    }

    return drawTextAt(
        "UP/DOWN  FUNCTION SELECT  BACK LAST PAGE",
        12,
        30,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderBookDetails(
    const AppState& app_state,
    const BookRecord& book
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const auto orientation =
        app_state.orientation;
    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "BOOK DETAILS",
            26,
            28,
            50,
            orientation
        )) {
        return false;
    }

    if (app_state.book_details.mode !=
        BookDetailsMode::Details) {
        const bool removing =
            app_state.book_details.mode ==
            BookDetailsMode::DeleteConfirm;

        if (!drawTextAt(
                removing
                    ? "DELETE THIS BOOK?"
                    : "RESTART READING?",
                24,
                32,
                116,
                orientation
            )) {
            return false;
        }

        const std::string copy =
            removing
                ? "THIS REMOVES THE LOCAL FILE AND READING DATA."
                : "READING POSITION AND PROGRESS WILL RESET.";

        if (!drawTextAt(
                copy,
                14,
                32,
                154,
                orientation
            )) {
            return false;
        }

        const bool confirmed =
            removing
                ? app_state.book_details.confirm_delete
                : app_state.book_details.confirm_restart;

        const int gap = 16;
        const int button_width =
            (logical_width - 64 - gap) / 2;
        const int top =
            orientation == Orientation::Portrait
                ? 250
                : 220;

        drawRect(
            24,
            top,
            button_width,
            54,
            orientation,
            confirmed ? 1 : 2
        );
        drawRect(
            24 + button_width + gap,
            top,
            button_width,
            54,
            orientation,
            confirmed ? 2 : 1
        );

        if (!drawTextAt(
                "CANCEL",
                17,
                40,
                top + 34,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                removing ? "DELETE" : "RESTART",
                17,
                40 + button_width + gap,
                top + 34,
                orientation
            )) {
            return false;
        }

        return drawTextAt(
            "UP/DOWN CHOOSE  FUNCTION CONFIRM  BACK CANCEL",
            12,
            28,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (!drawTextAt(
            book.metadata.title.empty()
                ? "UNTITLED"
                : book.metadata.title,
            24,
            30,
            98,
            orientation
        )) {
        return false;
    }

    if (!drawTextAt(
            book.metadata.author_display.empty()
                ? "UNKNOWN AUTHOR"
                : book.metadata.author_display,
            15,
            30,
            128,
            orientation
        )) {
        return false;
    }

    const char* state_label = "NEW";
    const char* primary = "START";

    if (book.reading_state == ReadingState::Reading) {
        state_label = "READING";
        primary = "CONTINUE";
    } else if (
        book.reading_state == ReadingState::Finished) {
        state_label = "FINISHED";
        primary = "READ AGAIN";
    }

    char progress[48] = {};
    std::snprintf(
        progress,
        sizeof(progress),
        "%s  %u%%",
        state_label,
        static_cast<unsigned>(
            std::min(
                100.0F,
                std::max(
                    0.0F,
                    book.progress * 100.0F
                )
            )
        )
    );

    if (!drawTextAt(
            progress,
            14,
            30,
            164,
            orientation
        )) {
        return false;
    }

    const std::string language =
        book.metadata.language.has_value()
            ? "LANGUAGE: " + *book.metadata.language
            : "LANGUAGE: UNKNOWN";

    if (!drawTextAt(
            language,
            13,
            30,
            192,
            orientation
        )) {
        return false;
    }

    std::string description =
        book.metadata.description.has_value() &&
        !book.metadata.description->empty()
            ? *book.metadata.description
            : "NO DESCRIPTION";

    const std::size_t description_limit =
        orientation == Orientation::Portrait
            ? 72U
            : 110U;

    if (description.size() > description_limit) {
        description.resize(description_limit);
        description += "...";
    }

    if (!drawTextAt(
            description,
            13,
            30,
            222,
            orientation
        )) {
        return false;
    }

    struct ActionRow {
        BookDetailsFocus focus;
        const char* label;
        bool visible;
    };

    const ActionRow actions[] = {
        {
            BookDetailsFocus::Primary,
            primary,
            true,
        },
        {
            BookDetailsFocus::RestartReading,
            "RESTART READING",
            book.reading_state != ReadingState::New ||
                book.progress > 0.0F,
        },
        {
            BookDetailsFocus::DeleteBook,
            "DELETE BOOK",
            true,
        },
    };

    int row = 0;
    const int start_y =
        orientation == Orientation::Portrait
            ? 300
            : 282;
    const int row_height = 58;

    for (const auto& action : actions) {
        if (!action.visible) {
            continue;
        }

        const int top =
            start_y + row * row_height;

        if (app_state.book_details.focus ==
            action.focus) {
            drawRect(
                24,
                top,
                logical_width - 48,
                row_height - 8,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                action.label,
                17,
                40,
                top + 32,
                orientation
            )) {
            return false;
        }

        ++row;
    }

    return drawTextAt(
        "UP/DOWN  FUNCTION SELECT  BACK LIBRARY",
        12,
        28,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

bool FreeTypeTextRenderer::renderReaderOverlay(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const Orientation orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    const auto& overlay =
        app_state.reader_overlay;

    const bool typography =
        overlay.mode ==
        ReaderOverlayMode::QuickTypography;

    if (!drawTextAt(
            typography ? "TYPOGRAPHY" : "READER MENU",
            26,
            30,
            52,
            orientation
        )) {
        return false;
    }

    if (typography) {
        constexpr const char* labels[] = {
            "SPACIOUS",
            "COMFORTABLE",
            "STANDARD",
            "COMPACT",
            "DENSE",
        };

        constexpr int kStartY = 94;
        constexpr int kRowHeight = 58;
        constexpr int kLeft = 24;

        for (int i = 0; i < 5; ++i) {
            const int top =
                kStartY + i * kRowHeight;

            if (overlay.focus_index ==
                static_cast<std::uint8_t>(i)) {
                drawRect(
                    kLeft,
                    top,
                    logical_width - 48,
                    kRowHeight - 8,
                    orientation,
                    2
                );
            }

            if (!drawTextAt(
                    labels[i],
                    18,
                    kLeft + 16,
                    top + 31,
                    orientation
                )) {
                return false;
            }
        }

        if (!drawTextAt(
                "UP/DOWN PREVIEW  FUNCTION APPLY  BACK CANCEL",
                12,
                30,
                orientation == Orientation::Portrait
                    ? 760
                    : 448,
                orientation
            )) {
            return false;
        }

        return true;
    }

    constexpr const char* labels[] = {
        "FONT",
        "CONTENTS & BOOKMARKS",
        "ADD BOOKMARK",
        "SEARCH IN BOOK",
        "ORIENTATION",
        "ABOUT BOOK",
        "SLEEP",
    };

    const int kStartY =
        orientation == Orientation::Portrait
            ? 92
            : 72;
    const int kRowHeight =
        orientation == Orientation::Portrait
            ? 64
            : 48;
    constexpr int kLeft = 24;

    for (int i = 0; i < 7; ++i) {
        const int top =
            kStartY + i * kRowHeight;

        if (overlay.focus_index ==
            static_cast<std::uint8_t>(i)) {
            drawRect(
                kLeft,
                top,
                logical_width - 48,
                kRowHeight - 8,
                orientation,
                2
            );
        }

        if (!drawTextAt(
                labels[i],
                orientation == Orientation::Portrait
                    ? 17
                    : 15,
                kLeft + 16,
                top + (
                    orientation == Orientation::Portrait
                        ? 34
                        : 29
                ),
                orientation
            )) {
            return false;
        }
    }

    return drawTextAt(
        "UP/DOWN  FUNCTION SELECT  BACK CLOSE",
        12,
        30,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}


bool FreeTypeTextRenderer::renderSearch(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

    if (app_state_ != nullptr &&
        !renderStatusBar(*app_state_)) {
        return false;
    }

    const Orientation orientation =
        app_state.orientation;
    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "SEARCH IN BOOK",
            26,
            30,
            52,
            orientation
        )) {
        return false;
    }

    drawRect(
        24,
        82,
        logical_width - 48,
        54,
        orientation,
        2
    );

    const std::string label =
        app_state.search.query.empty()
            ? "ENTER SEARCH QUERY"
            : app_state.search.query;

    if (!drawTextAt(
            label,
            18,
            40,
            116,
            orientation
        )) {
        return false;
    }

    if (app_state.keyboard.open) {
        if (!drawTextAt(
                "KEYBOARD",
                14,
                30,
                174,
                orientation
            )) {
            return false;
        }

        if (!drawKeyboardGrid(
                app_state.keyboard,
                orientation,
                orientation == Orientation::Portrait
                    ? 206
                    : 198
            )) {
            return false;
        }

        return drawTextAt(
            "UP/DOWN KEY  FUNCTION SELECT  BACK CLOSE",
            12,
            30,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (app_state.search.phase == SearchPhase::NoResults) {
        if (!drawTextAt(
                "NO RESULTS",
                20,
                30,
                190,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                "FUNCTION: EDIT QUERY",
                13,
                30,
                226,
                orientation
            )) {
            return false;
        }

        return drawTextAt(
            "BACK: QUERY  BACK AGAIN: READING",
            12,
            30,
            orientation == Orientation::Portrait
                ? 760
                : 448,
            orientation
        );
    }

    if (app_state.search.phase == SearchPhase::Results) {
        constexpr int kStartY = 198;
        constexpr int kRowHeight = 62;
        const std::size_t max_visible =
            orientation == Orientation::Portrait
                ? 8U
                : 4U;

        const std::size_t focus =
            std::min<std::size_t>(
                app_state.search.focus_index,
                app_state.search.matches.empty()
                    ? 0U
                    : app_state.search.matches.size() - 1U
            );

        const std::size_t visible_start =
            app_state.search.matches.empty()
                ? 0U
                : (focus / max_visible) * max_visible;

        const auto visible =
            std::min<std::size_t>(
                app_state.search.matches.size() -
                    visible_start,
                max_visible
            );

        char count_text[64] = {};
        const auto first_global =
            app_state.search.matches.empty()
                ? 0U
                : app_state.search.window_start +
                    static_cast<std::uint32_t>(
                        visible_start
                    ) +
                    1U;
        const auto last_global =
            app_state.search.window_start +
            static_cast<std::uint32_t>(
                visible_start + visible
            );

        std::snprintf(
            count_text,
            sizeof(count_text),
            "%lu-%lu OF %lu",
            static_cast<unsigned long>(first_global),
            static_cast<unsigned long>(last_global),
            static_cast<unsigned long>(
                app_state.search.total_matches
            )
        );

        if (!drawTextAt(
                count_text,
                14,
                30,
                174,
                orientation
            )) {
            return false;
        }

        for (std::size_t row = 0;
             row < visible;
             ++row) {
            const auto index =
                visible_start + row;
            const int top =
                kStartY +
                static_cast<int>(row) * kRowHeight;

            if (app_state.search.focus_index == index) {
                drawRect(
                    24,
                    top,
                    logical_width - 48,
                    kRowHeight - 6,
                    orientation,
                    2
                );
            }

            const auto& match =
                app_state.search.matches[index];

            if (!drawTextAt(
                    match.section_label,
                    12,
                    38,
                    top + 20,
                    orientation
                )) {
                return false;
            }

            if (!drawTextAt(
                    match.preview,
                    14,
                    38,
                    top + 43,
                    orientation
                )) {
                return false;
            }

            const auto start =
                static_cast<std::size_t>(
                    match.preview_match_start
                );
            const auto length =
                static_cast<std::size_t>(
                    match.preview_match_length
                );

            if (start <= match.preview.size() &&
                start + length <=
                    match.preview.size() &&
                length > 0U) {
                const auto prefix =
                    match.preview.substr(
                        0,
                        start
                    );
                const auto matched =
                    match.preview.substr(
                        start,
                        length
                    );

                const int underline_x =
                    38 +
                    static_cast<int>(
                        measureWidthPx(
                            prefix,
                            TypographySettings{
                                14,
                                1.0F,
                                0,
                            }
                        )
                    );

                const int underline_width =
                    static_cast<int>(
                        measureWidthPx(
                            matched,
                            TypographySettings{
                                14,
                                1.0F,
                                0,
                            }
                        )
                    );

                if (underline_width > 0) {
                    drawRect(
                        underline_x,
                        top + 47,
                        underline_width,
                        2,
                        orientation,
                        1
                    );
                }
            }
        }
    } else {
        if (!drawTextAt(
                "FUNCTION: OPEN KEYBOARD",
                13,
                30,
                178,
                orientation
            )) {
            return false;
        }
    }

    return drawTextAt(
        app_state.search.phase == SearchPhase::Results
            ? "BACK: EDIT QUERY"
            : "BACK: RETURN TO READING",
        12,
        30,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

} // namespace enku::platform::esp_idf
