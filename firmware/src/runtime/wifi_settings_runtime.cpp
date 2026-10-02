#include "enku/runtime/wifi_settings_runtime.hpp"

#include <array>

#include "enku/core/events.hpp"

namespace enku {
namespace {

constexpr std::array<WiFiPolicy, 3> kPolicies = {
    WiFiPolicy::Off,
    WiFiPolicy::Manual,
    WiFiPolicy::AutoConnectTrusted,
};

std::size_t policyIndex(
    WiFiPolicy policy
) {
    for (std::size_t i = 0;
         i < kPolicies.size();
         ++i) {
        if (kPolicies[i] == policy) {
            return i;
        }
    }

    return 0;
}

} // namespace

WiFiSettingsRuntime::WiFiSettingsRuntime(
    AppState& app_state,
    ApplicationStorageRuntime& storage,
    NetworkService& network,
    NetworkSettingsService& network_settings,
    SettingsNavigationRuntime& settings_nav,
    WiFiSettingsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      storage_(storage),
      network_(network),
      network_settings_(network_settings),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    app_state_.wifi_settings =
        WiFiSettingsState{};
    app_state_.wifi_settings.selected_policy =
        app_state_.wifi_policy;

    syncNetworkState();

    app_state_.screen = Screen::WiFiSettings;
    return render();
}

void WiFiSettingsRuntime::syncNetworkState() {
    app_state_.network.connected =
        network_.connected();

    const auto trusted =
        network_settings_.trustedSsid();

    app_state_.network.ssid =
        trusted.has_value()
            ? *trusted
            : std::string{};
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderWiFiSettings(
            app_state_
        )) {
        return WiFiSettingsRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return WiFiSettingsRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? WiFiSettingsRuntimeResult::Applied
        : WiFiSettingsRuntimeResult::Failed;
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::close() {
    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? WiFiSettingsRuntimeResult::Applied
        : WiFiSettingsRuntimeResult::Failed;
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::moveFocus(
    int direction
) {
    int focus =
        static_cast<int>(
            app_state_.wifi_settings.focus
        );

    focus =
        (focus + direction + 2) % 2;

    app_state_.wifi_settings.focus =
        static_cast<WiFiSettingsFocus>(focus);

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::beginPolicyEdit() {
    app_state_.wifi_settings.editing_policy = true;
    app_state_.wifi_settings.selected_policy =
        app_state_.wifi_policy;

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::cyclePolicy(
    int direction
) {
    int index =
        static_cast<int>(
            policyIndex(
                app_state_.wifi_settings.
                    selected_policy
            )
        );

    index =
        (index +
         direction +
         static_cast<int>(kPolicies.size())) %
        static_cast<int>(kPolicies.size());

    app_state_.wifi_settings.selected_policy =
        kPolicies[
            static_cast<std::size_t>(index)
        ];

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::applyPolicy() {
    const auto previous =
        app_state_.wifi_policy;

    const auto selected =
        app_state_.wifi_settings.
            selected_policy;

    if (storage_.settingsRuntime().handle(
            WiFiPolicyChanged{selected}
        ) != PersistStatus::Ok) {
        app_state_.wifi_settings.selected_policy =
            previous;
        return WiFiSettingsRuntimeResult::Failed;
    }

    const auto status =
        network_settings_.applyPolicy(selected);

    if (status ==
        NetworkPolicyStatus::DriverError) {
        storage_.settingsRuntime().handle(
            WiFiPolicyChanged{previous}
        );
        network_settings_.applyPolicy(previous);

        app_state_.wifi_settings.selected_policy =
            previous;
        syncNetworkState();
        return WiFiSettingsRuntimeResult::Failed;
    }

    if (status ==
        NetworkPolicyStatus::InvalidCredentials) {
        storage_.settingsRuntime().handle(
            WiFiPolicyChanged{previous}
        );
        network_settings_.applyPolicy(previous);

        app_state_.wifi_settings.selected_policy =
            previous;
        syncNetworkState();
        return WiFiSettingsRuntimeResult::Failed;
    }

    app_state_.wifi_settings.editing_policy = false;
    syncNetworkState();

    const auto rendered = render();

    if (rendered !=
        WiFiSettingsRuntimeResult::Applied) {
        return rendered;
    }

    return status ==
            NetworkPolicyStatus::NoTrustedNetwork
        ? WiFiSettingsRuntimeResult::
            NoTrustedNetwork
        : WiFiSettingsRuntimeResult::Applied;
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::cancelPolicyEdit() {
    app_state_.wifi_settings.selected_policy =
        app_state_.wifi_policy;
    app_state_.wifi_settings.editing_policy = false;

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::enterForgetConfirm() {
    app_state_.wifi_settings.forget_confirm = true;
    app_state_.wifi_settings.confirm_forget = false;

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::confirmForget() {
    if (!app_state_.wifi_settings.confirm_forget) {
        return cancelForget();
    }

    const auto status =
        network_settings_.forgetTrustedNetwork();

    if (status != NetworkPolicyStatus::Ok) {
        return WiFiSettingsRuntimeResult::Failed;
    }

    app_state_.wifi_settings.forget_confirm = false;
    app_state_.wifi_settings.confirm_forget = false;
    syncNetworkState();

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::cancelForget() {
    app_state_.wifi_settings.forget_confirm = false;
    app_state_.wifi_settings.confirm_forget = false;

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::WiFiSettings) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    if (app_state_.wifi_settings.forget_confirm) {
        switch (action) {
            case LogicalAction::NavigatePrevious:
            case LogicalAction::NavigateNext:
                app_state_.wifi_settings.confirm_forget =
                    !app_state_.wifi_settings.
                        confirm_forget;
                return render();

            case LogicalAction::Confirm:
                return confirmForget();

            case LogicalAction::Back:
                return cancelForget();

            default:
                return WiFiSettingsRuntimeResult::Ignored;
        }
    }

    if (app_state_.wifi_settings.editing_policy) {
        switch (action) {
            case LogicalAction::NavigatePrevious:
                return cyclePolicy(-1);

            case LogicalAction::NavigateNext:
                return cyclePolicy(1);

            case LogicalAction::Confirm:
                return applyPolicy();

            case LogicalAction::Back:
                return cancelPolicyEdit();

            default:
                return WiFiSettingsRuntimeResult::Ignored;
        }
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return moveFocus(-1);

        case LogicalAction::NavigateNext:
            return moveFocus(1);

        case LogicalAction::Confirm:
            if (app_state_.wifi_settings.focus ==
                WiFiSettingsFocus::Policy) {
                return beginPolicyEdit();
            }
            return enterForgetConfirm();

        case LogicalAction::Back:
            return close();

        default:
            return WiFiSettingsRuntimeResult::Ignored;
    }
}

} // namespace enku
