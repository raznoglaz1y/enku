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

    const bool search_mode =
        app_state.library.mode ==
        LibraryQueryMode::Search;

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
                : 6U);

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

        if (!search_mode &&
            !drawTextAt(
                "IMPORT A TXT BOOK TO START",
                14,
                32,
                start_y + 72,
                orientation
            )) {
            return false;
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

bool FreeTypeTextRenderer::renderSettingsScreen(
    const AppState& app_state
) {
    if (!ready()) {
        return false;
    }

    framebuffer_.clearWhite();

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

    if (book.metadata.series.has_value() &&
        !book.metadata.series->empty()) {
        std::string series =
            "SERIES: " + *book.metadata.series;

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
