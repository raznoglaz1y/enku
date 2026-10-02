#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
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

    GlobalSettings current() const;
    void apply(const GlobalSettings& settings);

private:
    AppState& app_state_;
    SettingsService& settings_;
};

} // namespace enku
