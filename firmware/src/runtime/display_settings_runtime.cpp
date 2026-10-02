#include "enku/runtime/display_settings_runtime.hpp"

namespace enku {

DisplaySettingsRuntime::DisplaySettingsRuntime(
    AppState& app_state,
    ApplicationReaderRuntime& reader,
    SettingsNavigationRuntime& settings_nav,
    DisplaySettingsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      reader_(reader),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return DisplaySettingsRuntimeResult::Ignored;
    }

    app_state_.display_settings.selected =
        app_state_.orientation;
    app_state_.screen = Screen::DisplaySettings;

    return render();
}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderDisplaySettings(
            app_state_
        )) {
        return DisplaySettingsRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return DisplaySettingsRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? DisplaySettingsRuntimeResult::Applied
        : DisplaySettingsRuntimeResult::Failed;
}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::close() {
    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? DisplaySettingsRuntimeResult::Applied
        : DisplaySettingsRuntimeResult::Failed;
}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::toggleSelection() {
    app_state_.display_settings.selected =
        app_state_.display_settings.selected ==
                Orientation::Portrait
            ? Orientation::Landscape
            : Orientation::Portrait;

    return render();
}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::applySelection() {
    const auto target =
        app_state_.display_settings.selected;

    if (reader_.applyOrientation(target) !=
        OrientationApplyStatus::Ok) {
        app_state_.display_settings.selected =
            app_state_.orientation;
        return DisplaySettingsRuntimeResult::Failed;
    }

    return render();
}

DisplaySettingsRuntimeResult
DisplaySettingsRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::DisplaySettings) {
        return DisplaySettingsRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
        case LogicalAction::NavigateNext:
            return toggleSelection();

        case LogicalAction::Confirm:
            return applySelection();

        case LogicalAction::Back:
            app_state_.display_settings.selected =
                app_state_.orientation;
            return close();

        default:
            return DisplaySettingsRuntimeResult::Ignored;
    }
}

} // namespace enku
