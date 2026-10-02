#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/input.hpp"
#include "input_runtime.hpp"
#include "library_runtime.hpp"
#include "library_search_runtime.hpp"
#include "book_details_runtime.hpp"
#include "book_finished_runtime.hpp"
#include "contents_bookmarks_runtime.hpp"
#include "about_book_runtime.hpp"
#include "settings_navigation_runtime.hpp"
#include "reading_settings_runtime.hpp"
#include "display_settings_runtime.hpp"
#include "locale_settings_runtime.hpp"
#include "about_device_runtime.hpp"
#include "power_off.hpp"
#include "reader_overlay_runtime.hpp"
#include "reader_runtime.hpp"
#include "search_runtime.hpp"
#include "sleep_wake.hpp"

namespace enku {

enum class InputDispatchResult : std::uint8_t {
    Ignored,
    Applied,
    Unhandled,
    Failed,
};

class InputDispatcher {
public:
    InputDispatcher(
        AppState& app_state,
        LibraryRuntimeController& library,
        ReaderRuntimeController& reader,
        SleepWakeCoordinator& sleep_wake,
        PowerOffCoordinator& power_off,
        ReaderOverlayRuntime* reader_overlay = nullptr,
        SearchRuntime* search = nullptr,
        LibrarySearchRuntime* library_search = nullptr,
        BookDetailsRuntime* book_details = nullptr,
        BookFinishedRuntime* book_finished = nullptr,
        ContentsBookmarksRuntime* contents_bookmarks = nullptr,
        AboutBookRuntime* about_book = nullptr,
        SettingsNavigationRuntime* settings_nav = nullptr,
        ReadingSettingsRuntime* reading_settings = nullptr,
        DisplaySettingsRuntime* display_settings = nullptr,
        LocaleSettingsRuntime* locale_settings = nullptr,
        AboutDeviceRuntime* about_device = nullptr
    );

    InputDispatchResult handle(
        const PhysicalInputEvent& input
    );

private:
    AppState& app_state_;
    LibraryRuntimeController& library_;
    ReaderRuntimeController& reader_;
    SleepWakeCoordinator& sleep_wake_;
    PowerOffCoordinator& power_off_;
    ReaderOverlayRuntime* reader_overlay_{nullptr};
    SearchRuntime* search_{nullptr};
    LibrarySearchRuntime* library_search_{nullptr};
    BookDetailsRuntime* book_details_{nullptr};
    BookFinishedRuntime* book_finished_{nullptr};
    ContentsBookmarksRuntime* contents_bookmarks_{nullptr};
    AboutBookRuntime* about_book_{nullptr};
    SettingsNavigationRuntime* settings_nav_{nullptr};
    ReadingSettingsRuntime* reading_settings_{nullptr};
    DisplaySettingsRuntime* display_settings_{nullptr};
    LocaleSettingsRuntime* locale_settings_{nullptr};
    AboutDeviceRuntime* about_device_{nullptr};
};

} // namespace enku
