#include "enku/runtime/about_device_runtime.hpp"

namespace enku {

AboutDeviceRuntime::AboutDeviceRuntime(
    AppState& app_state,
    SettingsNavigationRuntime& settings_nav,
    AboutDeviceRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

AboutDeviceRuntimeResult
AboutDeviceRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return AboutDeviceRuntimeResult::Ignored;
    }

    app_state_.screen = Screen::AboutDevice;
    return render();
}

AboutDeviceRuntimeResult
AboutDeviceRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderAboutDevice(
            app_state_
        )) {
        return AboutDeviceRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return AboutDeviceRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? AboutDeviceRuntimeResult::Applied
        : AboutDeviceRuntimeResult::Failed;
}

AboutDeviceRuntimeResult
AboutDeviceRuntime::close() {
    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? AboutDeviceRuntimeResult::Applied
        : AboutDeviceRuntimeResult::Failed;
}

AboutDeviceRuntimeResult
AboutDeviceRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::AboutDevice) {
        return AboutDeviceRuntimeResult::Ignored;
    }

    if (action == LogicalAction::Back ||
        action == LogicalAction::Confirm) {
        return close();
    }

    return AboutDeviceRuntimeResult::Ignored;
}

} // namespace enku
