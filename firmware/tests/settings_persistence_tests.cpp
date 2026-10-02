#include "enku/runtime/settings_runtime.hpp"
#include "enku/runtime/storage_startup.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/cbor_settings_service.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>

using namespace enku;

namespace {

void corruptLastByte(
    const std::filesystem::path& path
) {
    std::fstream file(
        path,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );
    assert(file);

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    assert(size > 0);

    file.seekg(size - std::streamoff(1));
    char value = 0;
    file.read(&value, 1);
    assert(file);

    value ^= static_cast<char>(0xFF);

    file.seekp(size - std::streamoff(1));
    file.write(&value, 1);
    file.flush();
    assert(file);
}

GlobalSettings customSettings() {
    GlobalSettings settings;
    settings.locale = LocaleId::Pl;
    settings.orientation = Orientation::Landscape;
    settings.library_view = LibraryView::List;
    settings.library_filter = LibraryFilter::Reading;
    settings.library_sort = LibrarySort::Author;
    settings.library_direction = SortDirection::Ascending;
    settings.reading_preset = ReadingPreset::Custom;
    settings.font_size_px = 22;
    settings.line_spacing = 1.55F;
    settings.margin_px = 32;
    settings.wifi_policy = WiFiPolicy::Manual;
    return settings;
}

} // namespace

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-settings-persistence-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    CborSettingsService service(state_files);

    GlobalSettings loaded;
    assert(
        service.load(loaded) ==
        PersistStatus::NotFound
    );

    // First generation A = defaults.
    const GlobalSettings defaults;
    assert(
        service.save(defaults) ==
        PersistStatus::Ok
    );

    // Second generation B = custom settings.
    const auto custom = customSettings();
    assert(
        service.save(custom) ==
        PersistStatus::Ok
    );

    assert(
        service.load(loaded) ==
        PersistStatus::Ok
    );
    assert(loaded.locale == LocaleId::Pl);
    assert(
        loaded.orientation ==
        Orientation::Landscape
    );
    assert(loaded.library_view == LibraryView::List);
    assert(
        loaded.library_filter ==
        LibraryFilter::Reading
    );
    assert(loaded.library_sort == LibrarySort::Author);
    assert(
        loaded.library_direction ==
        SortDirection::Ascending
    );
    assert(
        loaded.reading_preset ==
        ReadingPreset::Custom
    );
    assert(loaded.font_size_px == 22);
    assert(loaded.line_spacing == 1.55F);
    assert(loaded.margin_px == 32);
    assert(loaded.wifi_policy == WiFiPolicy::Manual);

    // Corrupt newest B generation: A remains a valid fallback.
    const auto slot_b =
        root / "system" / "settings.b.cbor";
    assert(std::filesystem::exists(slot_b));
    corruptLastByte(slot_b);

    assert(
        service.load(loaded) ==
        PersistStatus::Ok
    );
    assert(loaded.locale == LocaleId::En);
    assert(
        loaded.orientation ==
        Orientation::Portrait
    );

    // Corrupt both slots. SettingsRuntime must recover safe defaults instead
    // of forcing the whole device into Library/boot recovery.
    const auto slot_a =
        root / "system" / "settings.a.cbor";
    assert(std::filesystem::exists(slot_a));
    corruptLastByte(slot_a);

    AppState recovered_app;
    SettingsRuntimeController recovered_runtime(
        recovered_app,
        service
    );

    assert(
        recovered_runtime.loadAndApply() ==
        SettingsRuntimeStatus::DefaultsRecovered
    );
    assert(recovered_app.ui_locale == LocaleId::En);
    assert(
        recovered_app.orientation ==
        Orientation::Portrait
    );
    assert(
        recovered_app.typography.preset ==
        ReadingPreset::Standard
    );
    assert(
        recovered_app.wifi_policy ==
        WiFiPolicy::AutoConnectTrusted
    );

    // Persist non-default settings again and prove cold storage startup applies
    // them into a fresh AppState before Library becomes usable.
    assert(service.save(custom) == PersistStatus::Ok);

    AppState boot_app;
    CborSettingsService boot_settings_service(state_files);
    SettingsRuntimeController boot_settings(
        boot_app,
        boot_settings_service
    );

    PosixBookFileStore book_files(root);
    CborLibraryService library(state_files);
    BookImportService importer(library);

    StorageStartupCoordinator startup(
        boot_app,
        boot_settings,
        library,
        book_files,
        importer
    );

    const auto startup_result = startup.run();
    assert(startup_result.ready());
    assert(boot_app.screen == Screen::Library);
    assert(boot_app.ui_locale == LocaleId::Pl);
    assert(
        boot_app.orientation ==
        Orientation::Landscape
    );
    assert(boot_app.library.view == LibraryView::List);
    assert(
        boot_app.library.filter ==
        LibraryFilter::Reading
    );
    assert(boot_app.library.sort == LibrarySort::Author);
    assert(
        boot_app.library.direction ==
        SortDirection::Ascending
    );
    assert(
        boot_app.typography.preset ==
        ReadingPreset::Custom
    );
    assert(boot_app.typography.font_size_px == 22);
    assert(boot_app.typography.line_spacing == 1.55F);
    assert(boot_app.typography.margin_px == 32);
    assert(boot_app.wifi_policy == WiFiPolicy::Manual);

    std::filesystem::remove_all(root, ec);
    return 0;
}
