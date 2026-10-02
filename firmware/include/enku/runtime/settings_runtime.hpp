#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../services/services.hpp"

namespace enku {

enum class SettingsRuntimeStatus : std::uint8_t {
    Applied,
    DefaultsCreated,
    DefaultsRecovered,
    PersistenceFailure,
};

class SettingsRuntimeController {
public:
    SettingsRuntimeController(
        AppState& app_state,
        SettingsService& settings
    );

    SettingsRuntimeStatus loadAndApply();
    PersistStatus saveCurrent();

    PersistStatus handle(const LocaleChanged&);
    PersistStatus handle(const OrientationChanged&);
    PersistStatus handle(const LibraryViewChanged&);
    PersistStatus handle(const LibraryFilterChanged&);
    PersistStatus handle(const LibrarySortChanged&);
    PersistStatus handle(const TypographyDefaultsChanged&);
    PersistStatus handle(const WiFiPolicyChanged&);

    GlobalSettings current() const;
    void apply(const GlobalSettings& settings);

private:
    AppState& app_state_;
    SettingsService& settings_;
};

} // namespace enku
