#include "enku/runtime/application_reader_runtime.hpp"
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
            "iota kappa lambda mu nu xi omicron pi rho sigma tau."
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

    std::filesystem::remove_all(root, ec);
    return 0;
}
