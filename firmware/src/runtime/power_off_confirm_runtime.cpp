#include "enku/runtime/power_off_confirm_runtime.hpp"

namespace enku {

PowerOffConfirmRuntime::PowerOffConfirmRuntime(
    AppState& app_state,
    PowerOffCoordinator& power_off,
    SettingsNavigationRuntime& settings_nav,
    PowerOffConfirmRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      power_off_(power_off),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return PowerOffConfirmRuntimeResult::Ignored;
    }

    app_state_.power_off_confirm =
        PowerOffConfirmState{};
    app_state_.screen = Screen::PowerOffConfirm;

    return render();
}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderPowerOffConfirm(
            app_state_
        )) {
        return PowerOffConfirmRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return PowerOffConfirmRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? PowerOffConfirmRuntimeResult::Applied
        : PowerOffConfirmRuntimeResult::Failed;
}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::close() {
    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? PowerOffConfirmRuntimeResult::Applied
        : PowerOffConfirmRuntimeResult::Failed;
}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::toggle() {
    app_state_.power_off_confirm.focus =
        app_state_.power_off_confirm.focus ==
                PowerOffConfirmFocus::Cancel
            ? PowerOffConfirmFocus::PowerOff
            : PowerOffConfirmFocus::Cancel;

    return render();
}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::confirm() {
    if (app_state_.power_off_confirm.focus ==
        PowerOffConfirmFocus::Cancel) {
        return close();
    }

    return power_off_.powerOff() ==
        PowerOffStatus::Applied
        ? PowerOffConfirmRuntimeResult::Applied
        : PowerOffConfirmRuntimeResult::Failed;
}

PowerOffConfirmRuntimeResult
PowerOffConfirmRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::PowerOffConfirm) {
        return PowerOffConfirmRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
        case LogicalAction::NavigateNext:
            return toggle();

        case LogicalAction::Confirm:
            return confirm();

        case LogicalAction::Back:
            return close();

        default:
            return PowerOffConfirmRuntimeResult::Ignored;
    }
}

} // namespace enku
