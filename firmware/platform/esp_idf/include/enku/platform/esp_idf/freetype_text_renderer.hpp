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
#include "enku/render/search_renderer.hpp"
#include "enku/render/book_details_renderer.hpp"
#include "enku/render/book_finished_renderer.hpp"
#include "enku/render/contents_bookmarks_renderer.hpp"
#include "enku/render/about_book_renderer.hpp"
#include "enku/render/settings_screen_renderer.hpp"
#include "enku/render/reading_settings_renderer.hpp"
#include "enku/render/display_settings_renderer.hpp"
#include "enku/render/locale_settings_renderer.hpp"

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
      public ReaderOverlayRenderer,
      public SearchRenderer,
      public BookDetailsRenderer,
      public BookFinishedRenderer,
      public ContentsBookmarksRenderer,
      public AboutBookRenderer,
      public SettingsScreenRenderer,
      public ReadingSettingsRenderer,
      public DisplaySettingsRenderer,
      public LocaleSettingsRenderer {
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
    void bindAppState(const AppState& app_state);

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

    bool renderSearch(
        const AppState& app_state
    ) override;

    bool renderBookDetails(
        const AppState& app_state,
        const BookRecord& book
    ) override;

    bool renderBookFinished(
        const AppState& app_state,
        const BookRecord& book
    ) override;

    bool renderContentsBookmarks(
        const AppState& app_state,
        const std::vector<ContentsEntry>& contents,
        const std::vector<BookmarkRecord>& bookmarks
    ) override;

    bool renderAboutBook(
        const AppState& app_state,
        const BookRecord& book
    ) override;

    bool renderSettingsScreen(
        const AppState& app_state
    ) override;

    bool renderReadingSettings(
        const AppState& app_state
    ) override;

    bool renderDisplaySettings(
        const AppState& app_state
    ) override;

    bool renderLocaleSettings(
        const AppState& app_state
    ) override;

private:
    OwnedMonoFramebuffer& framebuffer_;
    const AppState* app_state_{nullptr};
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

    bool drawKeyboardGrid(
        const KeyboardState& keyboard,
        Orientation orientation,
        int top
    );
};

} // namespace enku::platform::esp_idf
