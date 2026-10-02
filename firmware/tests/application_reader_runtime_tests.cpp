#include "enku/runtime/application_reader_runtime.hpp"
#include "enku/runtime/reader_overlay_runtime.hpp"
#include "enku/runtime/search_runtime.hpp"
#include "enku/runtime/contents_bookmarks_runtime.hpp"
#include "enku/runtime/about_book_runtime.hpp"
#include "enku/runtime/settings_navigation_runtime.hpp"
#include "enku/runtime/reading_settings_runtime.hpp"
#include "enku/runtime/display_settings_runtime.hpp"
#include "enku/runtime/locale_settings_runtime.hpp"
#include "enku/runtime/about_device_runtime.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <filesystem>
#include <string_view>

using namespace enku;

namespace {

class FixedWidthRenderer final
    : public TextMeasurer,
      public ReaderPageRenderer,
      public LibraryPageRenderer {
public:
    std::uint16_t measureWidthPx(
        std::string_view text,
        const TypographySettings&
    ) const override {
        return static_cast<std::uint16_t>(
            text.size() * 8U
        );
    }

    std::uint16_t lineHeightPx(
        const TypographySettings&
    ) const override {
        return 18;
    }

    bool renderPage(
        const PageResult& page,
        const TypographySettings&,
        Orientation orientation
    ) override {
        last_orientation = orientation;
        ++renders;
        last_lines =
            static_cast<std::uint32_t>(
                page.lines.size()
            );
        return true;
    }

    bool renderLibrary(
        const AppState&,
        const LibraryPage& page
    ) override {
        ++library_renders;
        last_library_items =
            static_cast<std::uint32_t>(
                page.items.size()
            );
        return true;
    }

    std::uint32_t renders{0};
    std::uint32_t last_lines{0};
    Orientation last_orientation{Orientation::Landscape};
    std::uint32_t library_renders{0};
    std::uint32_t last_library_items{0};
};

class FakeRefreshService final : public RefreshService {
public:
    bool busy() const override {
        return false;
    }

    bool submit(
        const RefreshRequest& request
    ) override {
        ++submitted;
        last = request;
        return true;
    }

    void cancelObsolete(std::uint32_t) override {}

    RefreshStats stats() const override {
        return {};
    }

    std::uint32_t submitted{0};
    RefreshRequest last;
};

} // namespace

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-application-reader-runtime-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);

    ApplicationStorageRuntime storage(
        state_files,
        book_files
    );

    FixedWidthRenderer renderer;
    FakeRefreshService refresh;

    ApplicationReaderRuntime runtime(
        storage,
        refresh,
        renderer,
        renderer,
        renderer,
        TypographySettings{18, 1.2F, 16},
        Viewport{300, 420}
    );

    const auto boot = runtime.bootRestore().run();
    assert(
        boot.status ==
        BootRestoreStatus::LibraryReady
    );
    assert(
        storage.appState().screen ==
        Screen::Library
    );

    const std::string staged_path =
        "/system/tmp/reader.txt";

    assert(
        book_files.write(
            staged_path,
            "Alpha beta gamma delta epsilon zeta eta theta "
            "iota kappa lambda mu nu xi omicron pi rho sigma tau. "
            "beta beta beta beta beta beta beta beta beta beta "
            "beta beta beta beta beta beta beta beta beta beta "
            "beta beta beta beta beta beta beta beta beta. "
            "абвгдежзийклмнопрстуфхцчшщъыьэюя "
            "абвгдежзийклмнопрстуфхцчшщъыьэюя "
            "Привет мир привет."
        ) == BookFileStatus::Ok
    );

    const auto imported =
        storage.stagedImport().import(
            staged_path,
            "Reader.txt",
            1
        );

    assert(imported.ok());

    assert(
        runtime.library().handle(
            LibraryRefreshRequested{}
        ) == LibraryRuntimeResult::Applied
    );
    assert(renderer.library_renders == 1);
    assert(renderer.last_library_items == 1);

    storage.appState().library.focused_book =
        imported.book_id;

    assert(
        runtime.library().handle(
            OpenFocusedBookRequested{}
        ) == LibraryRuntimeResult::Applied
    );

    assert(
        storage.appState().screen ==
        Screen::Reading
    );
    assert(renderer.renders == 1);
    assert(renderer.last_lines > 0);
    assert(renderer.last_orientation == Orientation::Portrait);
    assert(refresh.submitted > 0);

    const auto before =
        storage.appState().reading_position;
    assert(before.has_value());

    assert(
        runtime.applyOrientation(
            Orientation::Landscape
        ) == OrientationApplyStatus::Ok
    );
    assert(
        storage.appState().orientation ==
        Orientation::Landscape
    );
    assert(
        renderer.last_orientation ==
        Orientation::Landscape
    );
    assert(
        storage.appState().reading_position.has_value()
    );
    assert(
        storage.appState().reading_position->text_offset ==
        before->text_offset
    );

    ReaderOverlayRuntime overlay(
        storage,
        runtime
    );

    ContentsBookmarksRuntime contents_bookmarks(
        storage.appState(),
        runtime,
        storage.bookmarks()
    );

    AboutBookRuntime about_book(
        storage.appState(),
        storage.library(),
        runtime
    );

    const auto overlay_position =
        storage.appState().reading_position;
    assert(overlay_position.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenQuickTypography
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::ReaderOverlay);

    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().typography.preset ==
        ReadingPreset::Compact
    );
    assert(
        storage.appState().reading_position.has_value()
    );
    assert(
        storage.appState().reading_position->text_offset ==
        overlay_position->text_offset
    );

    assert(
        overlay.handle(
            LogicalAction::Back
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().typography.preset ==
        ReadingPreset::Standard
    );
    assert(
        storage.appState().reading_position->text_offset ==
        overlay_position->text_offset
    );

    assert(
        overlay.handle(
            LogicalAction::OpenQuickTypography
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().typography.preset ==
        ReadingPreset::Compact
    );
    assert(
        storage.appState().reading_position->text_offset ==
        overlay_position->text_offset
    );

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().reader_overlay.mode ==
        ReaderOverlayMode::Menu
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().reader_overlay.mode ==
        ReaderOverlayMode::QuickTypography
    );
    assert(
        overlay.handle(
            LogicalAction::Back
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);

    // Add Bookmark from the approved Reader Menu stores the current
    // semantic position and returns to Reading. Repeating it is a no-op.
    const auto bookmark_position =
        storage.appState().reading_position;
    assert(bookmark_position.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 2; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        storage.appState().reader_overlay.focus_index == 2
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);

    std::vector<BookmarkRecord> saved_bookmarks;
    assert(
        storage.bookmarks().load(
            *storage.appState().current_book,
            saved_bookmarks
        ) == BookmarkStatus::Ok
    );
    assert(saved_bookmarks.size() == 1);
    assert(
        saved_bookmarks[0].position.text_offset ==
        bookmark_position->text_offset
    );

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 2; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::Applied
    );
    saved_bookmarks.clear();
    assert(
        storage.bookmarks().load(
            *storage.appState().current_book,
            saved_bookmarks
        ) == BookmarkStatus::Ok
    );
    assert(saved_bookmarks.size() == 1);

    // Contents & Bookmarks preserves the position until an item is
    // explicitly opened. TXT currently has no structured ToC, so the
    // Bookmarks tab remains available while Contents is empty.
    const auto before_overlay =
        storage.appState().reading_position;
    assert(before_overlay.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) ==
        ReaderOverlayRuntimeResult::ContentsBookmarksRequested
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        contents_bookmarks.openFromReader() ==
        ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::ContentsBookmarks
    );
    assert(contents_bookmarks.contents().empty());
    assert(contents_bookmarks.bookmarks().size() == 1);
    assert(
        storage.appState().reading_position->text_offset ==
        before_overlay->text_offset
    );

    assert(
        contents_bookmarks.handle(
            LogicalAction::NavigateNext
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        storage.appState().contents_bookmarks.focus ==
        ContentsBookmarksFocus::BookmarksTab
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::Confirm
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        storage.appState().contents_bookmarks.tab ==
        ContentsBookmarksTab::Bookmarks
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::NavigateNext
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        storage.appState().contents_bookmarks.focus ==
        ContentsBookmarksFocus::Item
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::Back
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().reading_position->text_offset ==
        before_overlay->text_offset
    );

    // Reopen and explicitly select the bookmark.
    assert(
        contents_bookmarks.openFromReader() ==
        ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::NavigateNext
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::Confirm
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::NavigateNext
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(
        contents_bookmarks.handle(
            LogicalAction::Confirm
        ) == ContentsBookmarksRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().reading_position->text_offset <=
        bookmark_position->text_offset
    );

    // About Book is read-only and returns to the exact same reading
    // position.
    const auto about_position =
        storage.appState().reading_position;
    assert(about_position.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 5; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        storage.appState().reader_overlay.focus_index == 5
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::AboutBookRequested
    );
    assert(storage.appState().screen == Screen::Reading);

    assert(
        about_book.openFromReader() ==
        AboutBookRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::AboutBook);
    assert(
        storage.appState().reading_position->text_offset ==
        about_position->text_offset
    );

    assert(
        about_book.handle(
            LogicalAction::Back
        ) == AboutBookRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().reading_position->text_offset ==
        about_position->text_offset
    );

    const auto orientation_position =
        storage.appState().reading_position;
    assert(orientation_position.has_value());
    const auto orientation_before =
        storage.appState().orientation;

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 4; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        storage.appState().reader_overlay.focus_index == 4
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        storage.appState().orientation != orientation_before
    );
    assert(
        renderer.last_orientation ==
        storage.appState().orientation
    );
    assert(
        storage.appState().reading_position.has_value()
    );
    assert(
        storage.appState().reading_position->text_offset ==
        orientation_position->text_offset
    );

    SearchRuntime search(
        storage,
        runtime
    );

    const auto search_origin =
        storage.appState().reading_position;
    assert(search_origin.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 3; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        storage.appState().reader_overlay.focus_index == 3
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::SearchRequested
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(
        search.openFromReader() ==
        SearchRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Search);
    assert(storage.appState().search.origin_position.has_value());
    assert(
        storage.appState().search.origin_position->text_offset ==
        search_origin->text_offset
    );

    assert(
        search.submitQuery("ПРИВЕТ") ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::Results
    );
    assert(storage.appState().search.total_matches == 2);
    assert(storage.appState().search.matches.size() == 2);
    assert(
        storage.appState().search.matches[0].preview_match_length ==
        std::string("Привет").size()
    );
    assert(
        storage.appState().search.matches[0].preview.find("Привет") !=
        std::string::npos
    );
    assert(
        (static_cast<unsigned char>(
            storage.appState().search.matches[0].preview[0]
        ) & 0xC0U) != 0x80U
    );
    assert(
        storage.appState().search.matches[0].preview.substr(
            storage.appState().search.matches[0].preview_match_start,
            storage.appState().search.matches[0].preview_match_length
        ) == "Привет"
    );

    assert(
        search.handle(LogicalAction::Back) ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::QueryEntry
    );

    assert(
        search.submitQuery("not-present") ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::NoResults
    );
    assert(storage.appState().search.total_matches == 0);
    assert(storage.appState().search.matches.empty());

    assert(
        search.handle(LogicalAction::Back) ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::QueryEntry
    );
    assert(storage.appState().screen == Screen::Search);
    assert(
        storage.appState().search.origin_position->text_offset ==
        search_origin->text_offset
    );

    assert(
        search.submitQuery("beta") ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::Results
    );
    assert(storage.appState().search.total_matches == 30);
    assert(storage.appState().search.matches.size() == 24);
    assert(storage.appState().search.window_start == 0);
    assert(
        !storage.appState().search.matches[0].section_label.empty()
    );
    assert(
        storage.appState().search.matches[0].preview_match_length ==
        4
    );
    assert(
        storage.appState().search.matches[0].preview.find("beta") !=
        std::string::npos
    );

    for (int i = 0; i < 23; ++i) {
        assert(
            search.handle(
                LogicalAction::NavigateNext
            ) == SearchRuntimeResult::Applied
        );
    }

    assert(storage.appState().search.focus_index == 23);

    assert(
        search.handle(
            LogicalAction::NavigateNext
        ) == SearchRuntimeResult::Applied
    );
    assert(storage.appState().search.window_start == 24);
    assert(storage.appState().search.focus_index == 0);
    assert(storage.appState().search.matches.size() == 6);

    assert(
        search.handle(LogicalAction::Back) ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::QueryEntry
    );
    assert(storage.appState().search.query == "beta");
    assert(storage.appState().search.matches.empty());
    assert(
        storage.appState().search.origin_position->text_offset ==
        search_origin->text_offset
    );

    assert(
        search.submitQuery("beta") ==
        SearchRuntimeResult::Applied
    );
    assert(
        storage.appState().search.phase ==
        SearchPhase::Results
    );
    assert(storage.appState().search.window_start == 0);
    assert(storage.appState().search.focus_index == 0);
    assert(storage.appState().search.matches.size() == 24);
    assert(
        storage.appState().search.matches[0].position.book_id ==
        *storage.appState().current_book
    );

    const auto selected_match =
        storage.appState().search.matches[0].position;

    assert(
        search.handle(LogicalAction::Confirm) ==
        SearchRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(storage.appState().search_highlight.position.has_value());
    assert(
        storage.appState().search_highlight.position->text_offset ==
        selected_match.text_offset
    );
    assert(
        storage.appState().search_highlight.query == "beta"
    );
    assert(storage.appState().reading_position.has_value());
    assert(
        storage.appState().reading_position->text_offset <=
        selected_match.text_offset
    );

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 3; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::SearchRequested
    );
    assert(
        search.openFromReader() ==
        SearchRuntimeResult::Applied
    );
    assert(
        search.handle(LogicalAction::Back) ==
        SearchRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Reading);
    assert(!storage.appState().search_highlight.position.has_value());

    assert(
        overlay.handle(
            LogicalAction::OpenReaderMenu
        ) == ReaderOverlayRuntimeResult::Applied
    );
    for (int i = 0; i < 6; ++i) {
        assert(
            overlay.handle(
                LogicalAction::NavigateNext
            ) == ReaderOverlayRuntimeResult::Applied
        );
    }
    assert(
        storage.appState().reader_overlay.focus_index == 6
    );
    assert(
        overlay.handle(
            LogicalAction::Confirm
        ) == ReaderOverlayRuntimeResult::SleepRequested
    );
    assert(storage.appState().screen == Screen::Reading);

    const auto next =
        runtime.reader().handle(
            PageNextRequested{}
        );

    assert(
        next == ReaderRuntimeResult::Applied ||
        next == ReaderRuntimeResult::EndOfBook
    );

    if (next == ReaderRuntimeResult::Applied) {
        assert(renderer.renders >= 2);
    }

    assert(
        runtime.reader().handle(
            BackRequested{}
        ) == ReaderRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::Library
    );

    SettingsNavigationRuntime settings_nav(
        storage.appState(),
        runtime.library()
    );

    ReadingSettingsRuntime reading_settings(
        storage.appState(),
        storage,
        runtime,
        settings_nav
    );

    DisplaySettingsRuntime display_settings(
        storage.appState(),
        runtime,
        settings_nav
    );

    LocaleSettingsRuntime locale_settings(
        storage.appState(),
        storage,
        settings_nav
    );

    AboutDeviceRuntime about_device(
        storage.appState(),
        settings_nav
    );

    const auto typography_before_settings =
        storage.appState().typography;

    assert(
        settings_nav.handle(
            OpenSettingsRequested{}
        ) == SettingsNavigationResult::Applied
    );
    assert(storage.appState().screen == Screen::Settings);
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Reading
    );

    assert(
        reading_settings.openFromSettings() ==
        ReadingSettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::ReadingSettings
    );

    assert(
        reading_settings.handle(
            LogicalAction::Confirm
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(storage.appState().reading_settings.editing);

    assert(
        reading_settings.handle(
            LogicalAction::NavigateNext
        ) == ReadingSettingsRuntimeResult::Applied
    );
    const auto committed_typography =
        storage.appState().typography;
    assert(
        committed_typography.preset !=
        typography_before_settings.preset ||
        committed_typography.font_size_px !=
        typography_before_settings.font_size_px ||
        committed_typography.line_spacing !=
        typography_before_settings.line_spacing ||
        committed_typography.margin_px !=
        typography_before_settings.margin_px
    );

    assert(
        reading_settings.handle(
            LogicalAction::Confirm
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(!storage.appState().reading_settings.editing);

    GlobalSettings persisted_typography;
    assert(
        storage.settingsStore().load(
            persisted_typography
        ) == PersistStatus::Ok
    );
    assert(
        persisted_typography.reading_preset ==
        committed_typography.preset
    );
    assert(
        persisted_typography.font_size_px ==
        committed_typography.font_size_px
    );

    assert(
        reading_settings.handle(
            LogicalAction::NavigateNext
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().reading_settings.focus ==
        ReadingSettingsItem::FontSize
    );

    assert(
        reading_settings.handle(
            LogicalAction::Confirm
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(
        reading_settings.handle(
            LogicalAction::NavigateNext
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().typography.preset ==
        ReadingPreset::Custom
    );
    assert(
        storage.appState().typography.font_size_px >=
        committed_typography.font_size_px
    );

    assert(
        reading_settings.handle(
            LogicalAction::Back
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(!storage.appState().reading_settings.editing);
    assert(
        storage.appState().typography.preset ==
        committed_typography.preset
    );
    assert(
        storage.appState().typography.font_size_px ==
        committed_typography.font_size_px
    );

    assert(
        reading_settings.handle(
            LogicalAction::Back
        ) == ReadingSettingsRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Settings);
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Reading
    );

    assert(
        settings_nav.handle(
            LogicalAction::NavigateNext
        ) == SettingsNavigationResult::Applied
    );
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Display
    );

    assert(
        display_settings.openFromSettings() ==
        DisplaySettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::DisplaySettings
    );
    assert(
        storage.appState().display_settings.selected ==
        storage.appState().orientation
    );

    const auto orientation_before_display =
        storage.appState().orientation;

    assert(
        display_settings.handle(
            LogicalAction::NavigateNext
        ) == DisplaySettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().display_settings.selected !=
        orientation_before_display
    );

    assert(
        display_settings.handle(
            LogicalAction::Confirm
        ) == DisplaySettingsRuntimeResult::Applied
    );
    const auto applied_display_orientation =
        storage.appState().orientation;
    assert(
        applied_display_orientation !=
        orientation_before_display
    );

    GlobalSettings persisted_display;
    assert(
        storage.settingsStore().load(
            persisted_display
        ) == PersistStatus::Ok
    );
    assert(
        persisted_display.orientation ==
        applied_display_orientation
    );

    assert(
        display_settings.handle(
            LogicalAction::NavigateNext
        ) == DisplaySettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().display_settings.selected ==
        orientation_before_display
    );

    assert(
        display_settings.handle(
            LogicalAction::Back
        ) == DisplaySettingsRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Settings);
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Display
    );
    assert(
        storage.appState().orientation ==
        applied_display_orientation
    );

    assert(
        settings_nav.handle(
            LogicalAction::NavigateNext
        ) == SettingsNavigationResult::Applied
    );
    assert(
        settings_nav.handle(
            LogicalAction::NavigateNext
        ) == SettingsNavigationResult::Applied
    );
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Language
    );

    assert(
        locale_settings.openFromSettings() ==
        LocaleSettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::LocaleSettings
    );
    assert(
        storage.appState().locale_settings.focus_index == 0
    );

    assert(
        locale_settings.handle(
            LogicalAction::NavigateNext
        ) == LocaleSettingsRuntimeResult::Applied
    );
    assert(
        storage.appState().locale_settings.focus_index == 1
    );

    assert(
        locale_settings.handle(
            LogicalAction::Confirm
        ) == LocaleSettingsRuntimeResult::Applied
    );
    assert(storage.appState().ui_locale == LocaleId::Ru);

    GlobalSettings persisted_locale;
    assert(
        storage.settingsStore().load(
            persisted_locale
        ) == PersistStatus::Ok
    );
    assert(persisted_locale.locale == LocaleId::Ru);

    assert(
        locale_settings.handle(
            LogicalAction::Back
        ) == LocaleSettingsRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Settings);
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::Language
    );

    assert(
        settings_nav.handle(
            LogicalAction::NavigateNext
        ) == SettingsNavigationResult::Applied
    );
    assert(
        settings_nav.handle(
            LogicalAction::NavigateNext
        ) == SettingsNavigationResult::Applied
    );
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::About
    );

    assert(
        about_device.openFromSettings() ==
        AboutDeviceRuntimeResult::Applied
    );
    assert(
        storage.appState().screen ==
        Screen::AboutDevice
    );

    assert(
        about_device.handle(
            LogicalAction::Back
        ) == AboutDeviceRuntimeResult::Applied
    );
    assert(storage.appState().screen == Screen::Settings);
    assert(
        storage.appState().settings_nav.focus ==
        SettingsItem::About
    );

    assert(
        settings_nav.handle(
            LogicalAction::Back
        ) == SettingsNavigationResult::Applied
    );
    assert(storage.appState().screen == Screen::Library);

    const auto library_renders_before_orientation =
        renderer.library_renders;

    const auto library_orientation_target =
        storage.appState().orientation ==
            Orientation::Portrait
            ? Orientation::Landscape
            : Orientation::Portrait;

    assert(
        runtime.applyOrientation(
            library_orientation_target
        ) == OrientationApplyStatus::Ok
    );
    assert(
        storage.appState().orientation ==
        library_orientation_target
    );
    assert(
        renderer.library_renders >
        library_renders_before_orientation
    );

    std::filesystem::remove_all(root, ec);
    return 0;
}
