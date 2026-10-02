#include "enku/runtime/application_reader_runtime.hpp"
#include "enku/runtime/reader_overlay_runtime.hpp"
#include "enku/runtime/search_runtime.hpp"
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
            "beta beta beta beta beta beta beta beta beta"
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
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().reader_overlay.focus_index == 1
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
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().reader_overlay.focus_index == 2
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
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
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
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        overlay.handle(
            LogicalAction::NavigateNext
        ) == ReaderOverlayRuntimeResult::Applied
    );
    assert(
        storage.appState().reader_overlay.focus_index == 3
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
