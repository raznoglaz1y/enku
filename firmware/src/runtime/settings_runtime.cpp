#include "enku/runtime/settings_runtime.hpp"

namespace enku {

SettingsRuntimeController::SettingsRuntimeController(
    AppState& app_state,
    SettingsService& settings
)
    : app_state_(app_state),
      settings_(settings) {}

GlobalSettings SettingsRuntimeController::current() const {
    GlobalSettings settings;
    settings.locale = app_state_.ui_locale;
    settings.orientation = app_state_.orientation;

    settings.library_view = app_state_.library.view;
    settings.library_filter = app_state_.library.filter;
    settings.library_sort = app_state_.library.sort;
    settings.library_direction =
        app_state_.library.direction;

    settings.reading_preset =
        app_state_.typography.preset;
    settings.font_size_px =
        app_state_.typography.font_size_px;
    settings.line_spacing =
        app_state_.typography.line_spacing;
    settings.margin_px =
        app_state_.typography.margin_px;

    settings.wifi_policy = app_state_.wifi_policy;
    return settings;
}

void SettingsRuntimeController::apply(
    const GlobalSettings& settings
) {
    app_state_.ui_locale = settings.locale;
    app_state_.orientation = settings.orientation;

    app_state_.library.view = settings.library_view;
    app_state_.library.filter = settings.library_filter;
    app_state_.library.sort = settings.library_sort;
    app_state_.library.direction =
        settings.library_direction;

    app_state_.typography.preset =
        settings.reading_preset;
    app_state_.typography.font_size_px =
        settings.font_size_px;
    app_state_.typography.line_spacing =
        settings.line_spacing;
    app_state_.typography.margin_px =
        settings.margin_px;

    app_state_.wifi_policy = settings.wifi_policy;
}

PersistStatus SettingsRuntimeController::saveCurrent() {
    return settings_.save(current());
}

SettingsRuntimeStatus
SettingsRuntimeController::loadAndApply() {
    GlobalSettings loaded;
    const auto status = settings_.load(loaded);

    if (status == PersistStatus::Ok) {
        apply(loaded);
        return SettingsRuntimeStatus::Applied;
    }

    if (status == PersistStatus::NotFound ||
        status == PersistStatus::InvalidRecord) {
        const GlobalSettings defaults;

        if (settings_.save(defaults) != PersistStatus::Ok) {
            return SettingsRuntimeStatus::PersistenceFailure;
        }

        apply(defaults);

        return status == PersistStatus::NotFound
            ? SettingsRuntimeStatus::DefaultsCreated
            : SettingsRuntimeStatus::DefaultsRecovered;
    }

    return SettingsRuntimeStatus::PersistenceFailure;
}

} // namespace enku
