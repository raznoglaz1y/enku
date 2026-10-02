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

const char* connectionFailureLabel(
    NetworkPolicyStatus status
) {
    switch (status) {
        case NetworkPolicyStatus::InvalidCredentials:
            return "INVALID CREDENTIALS";
        case NetworkPolicyStatus::ConnectionFailed:
            return "CONNECTION FAILED";
        case NetworkPolicyStatus::DriverError:
            return "WI-FI DRIVER ERROR";
        default:
            return "CONNECTION FAILED";
    }
}

} // namespace

WiFiSettingsRuntime::WiFiSettingsRuntime(
    AppState& app_state,
    ApplicationStorageRuntime& storage,
    NetworkService& network,
    NetworkSettingsService& network_settings,
    NetworkLifecycleCoordinator& network_lifecycle,
    SettingsNavigationRuntime& settings_nav,
    WiFiSettingsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      storage_(storage),
      network_(network),
      network_settings_(network_settings),
      network_lifecycle_(network_lifecycle),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh),
      keyboard_(app_state.keyboard) {}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    app_state_.wifi_settings =
        WiFiSettingsState{};
    app_state_.wifi_settings.selected_policy =
        app_state_.wifi_policy;
    app_state_.keyboard.open = false;
    app_state_.keyboard.focused_label.clear();

    syncNetworkState();

    app_state_.screen = Screen::WiFiSettings;
    return render();
}

void WiFiSettingsRuntime::syncNetworkState() {
    network_lifecycle_.sync();
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
    app_state_.keyboard.open = false;
    app_state_.wifi_settings.pending_password.clear();

    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? WiFiSettingsRuntimeResult::Applied
        : WiFiSettingsRuntimeResult::Failed;
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::moveFocus(
    int direction
) {
    constexpr int kFocusCount = 3;

    int focus =
        static_cast<int>(
            app_state_.wifi_settings.focus
        );

    focus =
        (focus + direction + kFocusCount) %
        kFocusCount;

    app_state_.wifi_settings.focus =
        static_cast<WiFiSettingsFocus>(focus);

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::scanNetworks() {
    auto& state = app_state_.wifi_settings;

    if (app_state_.wifi_policy == WiFiPolicy::Off) {
        state.status_message = "WI-FI IS OFF";
        state.scan_results.clear();
        state.network_focus = 0;
        state.selecting_network = false;
        return render();
    }

    state.status_message = "SCANNING...";
    state.scan_results.clear();
    state.network_focus = 0;

    const auto status =
        network_settings_.scanNetworks(
            state.scan_results
        );

    if (status != NetworkPolicyStatus::Ok) {
        state.status_message =
            connectionFailureLabel(status);
        state.selecting_network = false;
        return render();
    }

    if (state.scan_results.empty()) {
        state.status_message = "NO NETWORKS FOUND";
        state.selecting_network = false;
        return render();
    }

    state.status_message.clear();
    state.selecting_network = true;
    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::moveNetworkFocus(
    int direction
) {
    auto& state = app_state_.wifi_settings;

    if (state.scan_results.empty()) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    const int count =
        static_cast<int>(
            state.scan_results.size()
        );

    int focus =
        static_cast<int>(
            state.network_focus
        );

    focus =
        (focus + direction + count) % count;

    state.network_focus =
        static_cast<std::uint16_t>(focus);

    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::chooseNetwork() {
    auto& state = app_state_.wifi_settings;

    if (state.scan_results.empty() ||
        state.network_focus >=
            state.scan_results.size()) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    const auto& network_info =
        state.scan_results[
            state.network_focus
        ];

    state.pending_ssid =
        network_info.ssid;
    state.pending_password.clear();
    state.status_message.clear();

    if (!network_info.secured) {
        return connectPendingNetwork();
    }

    keyboard_.open(KeyboardMode::Latin);
    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::connectPendingNetwork() {
    auto& state = app_state_.wifi_settings;

    if (state.pending_ssid.empty()) {
        return WiFiSettingsRuntimeResult::Ignored;
    }

    state.status_message = "CONNECTING...";

    const auto status =
        network_settings_.connectToNetwork(
            state.pending_ssid,
            state.pending_password
        );

    if (status != NetworkPolicyStatus::Ok) {
        state.status_message =
            connectionFailureLabel(status);
        state.pending_password.clear();
        state.selecting_network = true;
        syncNetworkState();
        return render();
    }

    // Credentials become durable only after the transient connection
    // succeeds. This ordering is the key invariant of the flow.
    const auto persist_status =
        network_settings_.setTrustedNetwork(
            state.pending_ssid,
            state.pending_password
        );

    if (persist_status !=
        NetworkPolicyStatus::Ok) {
        network_.disconnect();
        state.status_message =
            "CONNECTED, SAVE FAILED";
        state.pending_password.clear();
        syncNetworkState();
        return render();
    }

    state.status_message = "CONNECTED";
    state.selecting_network = false;
    state.scan_results.clear();
    state.network_focus = 0;
    state.pending_ssid.clear();
    state.pending_password.clear();

    syncNetworkState();
    return render();
}

WiFiSettingsRuntimeResult
WiFiSettingsRuntime::cancelNetworkFlow() {
    auto& state = app_state_.wifi_settings;

    app_state_.keyboard.open = false;
    app_state_.keyboard.focused_label.clear();
    state.selecting_network = false;
    state.scan_results.clear();
    state.network_focus = 0;
    state.pending_ssid.clear();
    state.pending_password.clear();
    state.status_message.clear();

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

    network_lifecycle_.applyPolicy();
    const auto status =
        network_lifecycle_.lastPolicyStatus();

    if (status ==
            NetworkPolicyStatus::DriverError ||
        status ==
            NetworkPolicyStatus::InvalidCredentials ||
        status ==
            NetworkPolicyStatus::ConnectionFailed) {
        storage_.settingsRuntime().handle(
            WiFiPolicyChanged{previous}
        );
        network_lifecycle_.applyPolicy();

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

    if (app_state_.keyboard.open) {
        const auto result =
            keyboard_.handle(
                action,
                app_state_.wifi_settings.
                    pending_password
            );

        switch (result) {
            case KeyboardRuntimeResult::Changed:
                return render();

            case KeyboardRuntimeResult::Done:
                return connectPendingNetwork();

            case KeyboardRuntimeResult::Closed:
                app_state_.wifi_settings.
                    pending_password.clear();
                return render();

            case KeyboardRuntimeResult::Ignored:
            default:
                return WiFiSettingsRuntimeResult::Ignored;
        }
    }

    if (app_state_.wifi_settings.selecting_network) {
        switch (action) {
            case LogicalAction::NavigatePrevious:
                return moveNetworkFocus(-1);

            case LogicalAction::NavigateNext:
                return moveNetworkFocus(1);

            case LogicalAction::Confirm:
                return chooseNetwork();

            case LogicalAction::Back:
                return cancelNetworkFlow();

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
            switch (app_state_.wifi_settings.focus) {
                case WiFiSettingsFocus::Policy:
                    return beginPolicyEdit();

                case WiFiSettingsFocus::ScanNetworks:
                    return scanNetworks();

                case WiFiSettingsFocus::ForgetTrusted:
                    return enterForgetConfirm();

                default:
                    return WiFiSettingsRuntimeResult::Ignored;
            }

        case LogicalAction::Back:
            return close();

        default:
            return WiFiSettingsRuntimeResult::Ignored;
    }
}

} // namespace enku
