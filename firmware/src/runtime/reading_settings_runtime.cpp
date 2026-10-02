#include "enku/runtime/reading_settings_runtime.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "enku/core/events.hpp"
#include "enku/core/reader_overlay.hpp"

namespace enku {
namespace {

constexpr std::array<ReadingPreset, 5> kPresets = {
    ReadingPreset::Spacious,
    ReadingPreset::Comfortable,
    ReadingPreset::Standard,
    ReadingPreset::Compact,
    ReadingPreset::Dense,
};

constexpr std::uint16_t kMinFontSize = 12;
constexpr std::uint16_t kMaxFontSize = 32;
constexpr float kMinLineSpacing = 1.00F;
constexpr float kMaxLineSpacing = 2.00F;
constexpr float kLineSpacingStep = 0.05F;
constexpr std::uint16_t kMinMargin = 8;
constexpr std::uint16_t kMaxMargin = 64;
constexpr std::uint16_t kMarginStep = 2;

std::size_t presetIndex(
    ReadingPreset preset
) {
    for (std::size_t i = 0;
         i < kPresets.size();
         ++i) {
        if (kPresets[i] == preset) {
            return i;
        }
    }

    return 2;
}

} // namespace

ReadingSettingsRuntime::ReadingSettingsRuntime(
    AppState& app_state,
    ApplicationStorageRuntime& storage,
    ApplicationReaderRuntime& reader,
    SettingsNavigationRuntime& settings_nav,
    ReadingSettingsRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      storage_(storage),
      reader_(reader),
      settings_nav_(settings_nav),
      renderer_(renderer),
      refresh_(refresh) {}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::openFromSettings() {
    if (app_state_.screen != Screen::Settings) {
        return ReadingSettingsRuntimeResult::Ignored;
    }

    app_state_.reading_settings =
        ReadingSettingsState{};
    app_state_.reading_settings.baseline =
        app_state_.typography;
    app_state_.screen =
        Screen::ReadingSettings;

    return render();
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::render() {
    if (renderer_ != nullptr &&
        !renderer_->renderReadingSettings(
            app_state_
        )) {
        return ReadingSettingsRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return ReadingSettingsRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::ScreenChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? ReadingSettingsRuntimeResult::Applied
        : ReadingSettingsRuntimeResult::Failed;
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::close() {
    if (app_state_.reading_settings.editing) {
        return cancelEdit();
    }

    return settings_nav_.resume() ==
        SettingsNavigationResult::Applied
        ? ReadingSettingsRuntimeResult::Applied
        : ReadingSettingsRuntimeResult::Failed;
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::moveFocus(
    int direction
) {
    constexpr int kCount = 4;

    int focus =
        static_cast<int>(
            app_state_.reading_settings.focus
        );

    focus =
        (focus + direction + kCount) %
        kCount;

    app_state_.reading_settings.focus =
        static_cast<ReadingSettingsItem>(focus);

    return render();
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::beginEdit() {
    app_state_.reading_settings.baseline =
        app_state_.typography;
    app_state_.reading_settings.editing = true;

    return render();
}

ReadingPreset
ReadingSettingsRuntime::nextPreset(
    ReadingPreset current,
    int direction
) {
    int index =
        static_cast<int>(
            presetIndex(current)
        );

    index =
        (index +
         direction +
         static_cast<int>(kPresets.size())) %
        static_cast<int>(kPresets.size());

    return kPresets[
        static_cast<std::size_t>(index)
    ];
}

bool ReadingSettingsRuntime::applyPreview(
    const TypographyState& typography
) {
    const auto previous =
        app_state_.typography;

    app_state_.typography = typography;

    const auto result =
        reader_.reader().handle(
            TypographyDefaultsChanged{
                typography.preset,
                typography.font_size_px,
                typography.line_spacing,
                typography.margin_px,
            }
        );

    if (result != ReaderRuntimeResult::Applied &&
        result != ReaderRuntimeResult::Ignored) {
        app_state_.typography = previous;
        reader_.reader().handle(
            TypographyDefaultsChanged{
                previous.preset,
                previous.font_size_px,
                previous.line_spacing,
                previous.margin_px,
            }
        );
        return false;
    }

    return true;
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::adjust(
    int direction
) {
    auto next =
        app_state_.typography;

    switch (app_state_.reading_settings.focus) {
        case ReadingSettingsItem::Preset: {
            const auto preset =
                nextPreset(
                    next.preset,
                    direction
                );

            const auto values =
                readingPresetValues(preset);

            next.preset = preset;
            next.font_size_px =
                values.font_size_px;
            next.line_spacing =
                values.line_spacing;
            next.margin_px =
                values.margin_px;
            break;
        }

        case ReadingSettingsItem::FontSize: {
            const int candidate =
                static_cast<int>(
                    next.font_size_px
                ) + direction;

            next.font_size_px =
                static_cast<std::uint16_t>(
                    std::clamp(
                        candidate,
                        static_cast<int>(
                            kMinFontSize
                        ),
                        static_cast<int>(
                            kMaxFontSize
                        )
                    )
                );
            next.preset =
                ReadingPreset::Custom;
            break;
        }

        case ReadingSettingsItem::LineSpacing: {
            const float candidate =
                next.line_spacing +
                static_cast<float>(direction) *
                    kLineSpacingStep;

            next.line_spacing =
                std::clamp(
                    candidate,
                    kMinLineSpacing,
                    kMaxLineSpacing
                );

            next.line_spacing =
                std::round(
                    next.line_spacing * 100.0F
                ) / 100.0F;

            next.preset =
                ReadingPreset::Custom;
            break;
        }

        case ReadingSettingsItem::Margins: {
            const int candidate =
                static_cast<int>(
                    next.margin_px
                ) +
                direction *
                    static_cast<int>(
                        kMarginStep
                    );

            next.margin_px =
                static_cast<std::uint16_t>(
                    std::clamp(
                        candidate,
                        static_cast<int>(
                            kMinMargin
                        ),
                        static_cast<int>(
                            kMaxMargin
                        )
                    )
                );
            next.preset =
                ReadingPreset::Custom;
            break;
        }
    }

    if (!applyPreview(next)) {
        return ReadingSettingsRuntimeResult::Failed;
    }

    return render();
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::commitEdit() {
    const auto current =
        app_state_.typography;

    const auto status =
        storage_.settingsRuntime().handle(
            TypographyDefaultsChanged{
                current.preset,
                current.font_size_px,
                current.line_spacing,
                current.margin_px,
            }
        );

    if (status != PersistStatus::Ok) {
        if (!applyPreview(
                app_state_.reading_settings.baseline
            )) {
            return ReadingSettingsRuntimeResult::Failed;
        }

        app_state_.reading_settings.editing = false;
        return render();
    }

    app_state_.reading_settings.baseline =
        current;
    app_state_.reading_settings.editing = false;

    return render();
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::cancelEdit() {
    const auto baseline =
        app_state_.reading_settings.baseline;

    if (!applyPreview(baseline)) {
        return ReadingSettingsRuntimeResult::Failed;
    }

    app_state_.reading_settings.editing = false;

    return render();
}

ReadingSettingsRuntimeResult
ReadingSettingsRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::ReadingSettings) {
        return ReadingSettingsRuntimeResult::Ignored;
    }

    if (app_state_.reading_settings.editing) {
        switch (action) {
            case LogicalAction::NavigatePrevious:
                return adjust(-1);

            case LogicalAction::NavigateNext:
                return adjust(1);

            case LogicalAction::Confirm:
                return commitEdit();

            case LogicalAction::Back:
                return cancelEdit();

            default:
                return ReadingSettingsRuntimeResult::Ignored;
        }
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return moveFocus(-1);

        case LogicalAction::NavigateNext:
            return moveFocus(1);

        case LogicalAction::Confirm:
            return beginEdit();

        case LogicalAction::Back:
            return close();

        default:
            return ReadingSettingsRuntimeResult::Ignored;
    }
}

} // namespace enku
