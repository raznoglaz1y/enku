#include "enku/runtime/settings_navigation_runtime.hpp"

namespace enku {

SettingsNavigationRuntime::SettingsNavigationRuntime(
    AppState& app_state,
    LibraryRuntimeController& library,
    SettingsScreenRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      library_(library),
      renderer_(renderer),
      refresh_(refresh) {}

SettingsNavigationResult
SettingsNavigationRuntime::handle(
    const OpenSettingsRequested&
) {
    return open();
}

SettingsNavigationResult
SettingsNavigationRuntime::open() {
    if (app_state_.screen != Screen::Library) {
        return SettingsNavigationResult::Ignored;
    }

    app_state_.settings_nav =
        SettingsNavigationState{};
    app_state_.screen = Screen::Settings;

    return render();
}

SettingsNavigationResult
SettingsNavigationRuntime::close() {
    app_state_.screen = Screen::Library;

    const auto result =
        library_.redraw(
            RefreshReason::ScreenChanged,
            RefreshClass::Full
        );

    return result == LibraryRuntimeResult::Applied ||
           result == LibraryRuntimeResult::Empty
        ? SettingsNavigationResult::Applied
        : SettingsNavigationResult::Failed;
}

SettingsNavigationResult
SettingsNavigationRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderSettingsScreen(
            app_state_
        )) {
        return SettingsNavigationResult::Failed;
    }

    if (refresh_ == nullptr) {
        return SettingsNavigationResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? SettingsNavigationResult::Applied
        : SettingsNavigationResult::Failed;
}

SettingsNavigationResult
SettingsNavigationRuntime::moveFocus(
    int direction
) {
    constexpr int kCount = 8;

    int current =
        static_cast<int>(
            app_state_.settings_nav.focus
        );

    current =
        (current + direction + kCount) %
        kCount;

    app_state_.settings_nav.focus =
        static_cast<SettingsItem>(current);

    return render();
}

SettingsNavigationResult
SettingsNavigationRuntime::activate() {
    switch (app_state_.settings_nav.focus) {
        case SettingsItem::Reading:
            return SettingsNavigationResult::
                ReadingRequested;

        case SettingsItem::Display:
            return SettingsNavigationResult::
                DisplayRequested;

        case SettingsItem::WiFi:
            return SettingsNavigationResult::
                WiFiRequested;

        case SettingsItem::Language:
            return SettingsNavigationResult::
                LanguageRequested;

        case SettingsItem::Storage:
            return SettingsNavigationResult::
                StorageRequested;

        case SettingsItem::Sleep:
            return SettingsNavigationResult::
                SleepRequested;

        case SettingsItem::About:
            return SettingsNavigationResult::
                AboutRequested;

        case SettingsItem::PowerOff:
            return SettingsNavigationResult::
                PowerOffRequested;
    }

    return SettingsNavigationResult::Ignored;
}

SettingsNavigationResult
SettingsNavigationRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::Settings) {
        return SettingsNavigationResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return moveFocus(-1);

        case LogicalAction::NavigateNext:
            return moveFocus(1);

        case LogicalAction::Confirm:
            return activate();

        case LogicalAction::Back:
            return close();

        default:
            return SettingsNavigationResult::Ignored;
    }
}

} // namespace enku
