#include "enku/platform/esp_idf/freetype_text_renderer.hpp"

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

bool FreeTypeTextRenderer::renderPage(
    const PageResult& page,
    const TypographySettings& typography,
    Orientation orientation
) {
    if (!ensureSize(typography.font_size_px)) {
        return false;
    }

    framebuffer_.clearWhite();

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

    const Orientation orientation =
        app_state.orientation;

    const int logical_width =
        orientation == Orientation::Portrait
            ? 480
            : 800;

    if (!drawTextAt(
            "LIBRARY",
            28,
            32,
            48,
            orientation
        )) {
        return false;
    }

    char count_text[48] = {};
    std::snprintf(
        count_text,
        sizeof(count_text),
        "%lu BOOKS",
        static_cast<unsigned long>(
            page.total_matches
        )
    );

    if (!drawTextAt(
            count_text,
            14,
            logical_width - 150,
            42,
            orientation
        )) {
        return false;
    }

    constexpr int kStartY = 78;
    constexpr int kRowHeight = 64;
    constexpr int kLeft = 24;
    const int kWidth = logical_width - 48;
    const std::size_t kMaxVisible =
        orientation == Orientation::Portrait
            ? 10U
            : 6U;

    const auto visible =
        std::min<std::size_t>(
            page.items.size(),
            kMaxVisible
        );

    for (std::size_t i = 0; i < visible; ++i) {
        const auto& book = page.items[i];
        const int top =
            kStartY +
            static_cast<int>(i) *
                kRowHeight;

        const bool focused =
            app_state.library.focused_book.has_value() &&
            *app_state.library.focused_book ==
                book.book_id;

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

        std::string meta =
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
                "NO BOOKS",
                18,
                32,
                120,
                orientation
            )) {
            return false;
        }

        if (!drawTextAt(
                "IMPORT A TXT BOOK TO START",
                14,
                32,
                150,
                orientation
            )) {
            return false;
        }
    }

    return true;
}

} // namespace enku::platform::esp_idf

namespace enku::platform::esp_idf {

bool FreeTypeTextRenderer::renderReaderOverlay(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

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
        "TYPOGRAPHY",
        "ORIENTATION",
        "SEARCH",
        "SLEEP",
    };

    constexpr int kStartY = 98;
    constexpr int kRowHeight = 64;
    constexpr int kLeft = 24;

    for (int i = 0; i < 4; ++i) {
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
                top + 34,
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

        const auto key_count =
            keyboardKeyCount(
                app_state.keyboard.mode
            );
        const auto character_count =
            keyboardCharacterKeyCount(
                app_state.keyboard.mode
            );

        const int keyboard_top =
            orientation == Orientation::Portrait
                ? 206
                : 198;
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
                keyboard_top +
                row *
                    (key_height + gap);

            drawRect(
                x,
                y,
                character_width,
                key_height,
                orientation,
                app_state.keyboard.focus_index == i
                    ? 2
                    : 1
            );

            const auto key =
                keyboardKeyLabel(
                    app_state.keyboard,
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
            keyboard_top +
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
                app_state.keyboard.focus_index == i
                    ? 2
                    : 1
            );

            const auto key =
                keyboardKeyLabel(
                    app_state.keyboard,
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

    if (app_state.search.phase == SearchPhase::Results) {
        char count_text[64] = {};
        const auto visible_end =
            app_state.search.window_start +
            static_cast<std::uint32_t>(
                app_state.search.matches.size()
            );

        std::snprintf(
            count_text,
            sizeof(count_text),
            "%lu-%lu OF %lu",
            static_cast<unsigned long>(
                app_state.search.matches.empty()
                    ? 0U
                    : app_state.search.window_start + 1U
            ),
            static_cast<unsigned long>(
                visible_end
            ),
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

        constexpr int kStartY = 198;
        constexpr int kRowHeight = 62;
        const std::size_t max_visible =
            orientation == Orientation::Portrait
                ? 8U
                : 4U;

        const auto visible =
            std::min<std::size_t>(
                app_state.search.matches.size(),
                max_visible
            );

        for (std::size_t i = 0; i < visible; ++i) {
            const int top =
                kStartY +
                static_cast<int>(i) * kRowHeight;

            if (app_state.search.focus_index == i) {
                drawRect(
                    24,
                    top,
                    logical_width - 48,
                    kRowHeight - 6,
                    orientation,
                    2
                );
            }

            if (!drawTextAt(
                    app_state.search.matches[i].preview,
                    14,
                    38,
                    top + 32,
                    orientation
                )) {
                return false;
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
        "BACK: RETURN TO READING",
        12,
        30,
        orientation == Orientation::Portrait
            ? 760
            : 448,
        orientation
    );
}

} // namespace enku::platform::esp_idf
