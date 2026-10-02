#include "enku/runtime/locale_settings_runtime.hpp"

#include <array>

#include "enku/core/events.hpp"

namespace enku {
namespace {

constexpr std::array<LocaleId, 7> kLocales = {
    LocaleId::En,
    LocaleId::Ru,
    LocaleId::Pl,
    LocaleId::De,
    LocaleId::Fr,
    LocaleId::Es,
    LocaleId::It,
};

std::uint8_t localeIndex(
    LocaleId locale
) {
    for (std::size_t i = 0;
         i < kLocales.size();
         ++i) {
        if (kLocales[i] == locale) {
            return static_cast<std::uint8_t>(i);
        }
    }

    return 0;
}

} // namespace

LocaleSettingsRuntime::LocaleSettingsRuntime(
    AppState& app_state,
    ApplicationStorageRuntime& storage,
    SettingsNavigationRuntime& settings_nav,
    LocaleSettingsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      storage_(storage),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return LocaleSettingsRuntimeResult::Ignored;
    }

    app_state_.locale_settings.focus_index =
        localeIndex(app_state_.ui_locale);
    app_state_.locale_settings.window_start = 0;
    normalizeWindow();

    app_state_.screen = Screen::LocaleSettings;
    return render();
}

void LocaleSettingsRuntime::normalizeWindow() {
    constexpr std::uint8_t kVisiblePortrait = 7;
    constexpr std::uint8_t kVisibleLandscape = 5;

    const auto visible =
        app_state_.orientation ==
                Orientation::Portrait
            ? kVisiblePortrait
            : kVisibleLandscape;

    auto& window =
        app_state_.locale_settings.window_start;

    const auto focus =
        app_state_.locale_settings.focus_index;

    if (focus < window) {
        window = focus;
    } else if (
        focus >=
        static_cast<std::uint8_t>(
            window + visible
        )
    ) {
        window =
            static_cast<std::uint8_t>(
                focus - visible + 1U
            );
    }
}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::move(
    int direction
) {
    constexpr int kCount =
        static_cast<int>(kLocales.size());

    int index =
        static_cast<int>(
            app_state_.locale_settings.focus_index
        );

    index =
        (index + direction + kCount) %
        kCount;

    app_state_.locale_settings.focus_index =
        static_cast<std::uint8_t>(index);

    normalizeWindow();
    return render();
}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::apply() {
    const auto index =
        app_state_.locale_settings.focus_index;

    if (index >= kLocales.size()) {
        return LocaleSettingsRuntimeResult::Failed;
    }

    const auto status =
        storage_.settingsRuntime().handle(
            LocaleChanged{
                kLocales[index]
            }
        );

    if (status != PersistStatus::Ok) {
        app_state_.locale_settings.focus_index =
            localeIndex(app_state_.ui_locale);
        normalizeWindow();
        return LocaleSettingsRuntimeResult::Failed;
    }

    return render();
}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::close() {
    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? LocaleSettingsRuntimeResult::Applied
        : LocaleSettingsRuntimeResult::Failed;
}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderLocaleSettings(
            app_state_
        )) {
        return LocaleSettingsRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return LocaleSettingsRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? LocaleSettingsRuntimeResult::Applied
        : LocaleSettingsRuntimeResult::Failed;
}

LocaleSettingsRuntimeResult
LocaleSettingsRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::LocaleSettings) {
        return LocaleSettingsRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return move(-1);

        case LogicalAction::NavigateNext:
            return move(1);

        case LogicalAction::Confirm:
            return apply();

        case LogicalAction::Back:
            return close();

        default:
            return LocaleSettingsRuntimeResult::Ignored;
    }
}

} // namespace enku
