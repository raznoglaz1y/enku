#include "enku/runtime/reader_overlay_runtime.hpp"

#include <array>

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

std::size_t presetIndex(ReadingPreset preset) {
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        if (kPresets[i] == preset) {
            return i;
        }
    }

    return 2;
}

ReaderOverlayRuntimeResult toOverlayResult(
    ReaderRuntimeResult result
) {
    return result == ReaderRuntimeResult::Applied
        ? ReaderOverlayRuntimeResult::Applied
        : ReaderOverlayRuntimeResult::Failed;
}

} // namespace

ReaderOverlayRuntime::ReaderOverlayRuntime(
    ApplicationStorageRuntime& storage,
    ApplicationReaderRuntime& reader
)
    : storage_(storage),
      reader_(reader) {}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::handle(
    LogicalAction action
) {
    auto& app = storage_.appState();

    if (action == LogicalAction::OpenReaderMenu) {
        return openMenu();
    }

    if (action == LogicalAction::OpenQuickTypography) {
        return openQuickTypography();
    }

    if (app.screen != Screen::ReaderOverlay) {
        return ReaderOverlayRuntimeResult::Ignored;
    }

    switch (action) {
        case LogicalAction::NavigatePrevious:
            return navigate(-1);

        case LogicalAction::NavigateNext:
            return navigate(1);

        case LogicalAction::Confirm:
            return confirm();

        case LogicalAction::Back:
            return close();

        default:
            return ReaderOverlayRuntimeResult::Ignored;
    }
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::openMenu() {
    auto& app = storage_.appState();

    if (app.screen != Screen::Reading) {
        return ReaderOverlayRuntimeResult::Ignored;
    }

    baseline_ =
        storage_.settingsRuntime().current();
    baseline_valid_ = true;

    app.reader_overlay.mode =
        ReaderOverlayMode::Menu;
    app.reader_overlay.focus_index = 0;
    app.reader_overlay.preview_preset =
        app.typography.preset;
    app.screen = Screen::ReaderOverlay;

    return ReaderOverlayRuntimeResult::Applied;
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::openQuickTypography() {
    auto& app = storage_.appState();

    if (app.screen != Screen::Reading) {
        return ReaderOverlayRuntimeResult::Ignored;
    }

    baseline_ =
        storage_.settingsRuntime().current();
    baseline_valid_ = true;

    app.screen = Screen::ReaderOverlay;
    enterQuickTypography();

    return ReaderOverlayRuntimeResult::Applied;
}

void ReaderOverlayRuntime::enterQuickTypography() {
    auto& overlay =
        storage_.appState().reader_overlay;

    overlay.mode =
        ReaderOverlayMode::QuickTypography;

    const auto current =
        storage_.appState().typography.preset;

    const auto index = presetIndex(current);

    overlay.focus_index =
        static_cast<std::uint8_t>(index);
    overlay.preview_preset =
        kPresets[index];
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::navigate(int direction) {
    auto& overlay =
        storage_.appState().reader_overlay;

    if (overlay.mode == ReaderOverlayMode::Menu) {
        constexpr int kMenuCount = 4;

        int next =
            static_cast<int>(overlay.focus_index) +
            direction;

        if (next < 0) {
            next = kMenuCount - 1;
        } else if (next >= kMenuCount) {
            next = 0;
        }

        overlay.focus_index =
            static_cast<std::uint8_t>(next);

        return ReaderOverlayRuntimeResult::Applied;
    }

    int next =
        static_cast<int>(
            presetIndex(overlay.preview_preset)
        ) + direction;

    if (next < 0) {
        next =
            static_cast<int>(kPresets.size()) - 1;
    } else if (
        next >= static_cast<int>(kPresets.size())
    ) {
        next = 0;
    }

    return previewPreset(
        kPresets[static_cast<std::size_t>(next)]
    );
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::previewPreset(
    ReadingPreset preset
) {
    if (!baseline_valid_) {
        return ReaderOverlayRuntimeResult::Failed;
    }

    auto& app = storage_.appState();
    auto& overlay = app.reader_overlay;

    const auto values =
        readingPresetValues(preset);

    auto preview = baseline_;
    preview.reading_preset = preset;
    preview.font_size_px = values.font_size_px;
    preview.line_spacing = values.line_spacing;
    preview.margin_px = values.margin_px;

    storage_.settingsRuntime().apply(preview);

    const auto result =
        reader_.reader().handle(
            TypographyDefaultsChanged{
                preset,
                values.font_size_px,
                values.line_spacing,
                values.margin_px,
            }
        );

    if (result != ReaderRuntimeResult::Applied) {
        storage_.settingsRuntime().apply(baseline_);

        reader_.reader().handle(
            TypographyDefaultsChanged{
                baseline_.reading_preset,
                baseline_.font_size_px,
                baseline_.line_spacing,
                baseline_.margin_px,
            }
        );

        return ReaderOverlayRuntimeResult::Failed;
    }

    overlay.preview_preset = preset;
    overlay.focus_index =
        static_cast<std::uint8_t>(
            presetIndex(preset)
        );

    return ReaderOverlayRuntimeResult::Applied;
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::confirm() {
    auto& app = storage_.appState();
    auto& overlay = app.reader_overlay;

    if (overlay.mode == ReaderOverlayMode::Menu) {
        const auto item =
            static_cast<ReaderMenuItem>(
                overlay.focus_index
            );

        if (item == ReaderMenuItem::Typography) {
            enterQuickTypography();
            return ReaderOverlayRuntimeResult::Applied;
        }

        return ReaderOverlayRuntimeResult::Ignored;
    }

    if (!baseline_valid_) {
        return ReaderOverlayRuntimeResult::Failed;
    }

    const auto selected =
        overlay.preview_preset;

    // Ensure applyTypographyPreset() sees the durable pre-preview
    // settings as its rollback baseline.
    storage_.settingsRuntime().apply(baseline_);

    const auto status =
        reader_.applyTypographyPreset(selected);

    if (status != TypographyApplyStatus::Ok) {
        return ReaderOverlayRuntimeResult::Failed;
    }

    baseline_valid_ = false;
    app.screen = Screen::Reading;

    return ReaderOverlayRuntimeResult::Applied;
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::restoreBaseline() {
    if (!baseline_valid_) {
        return ReaderOverlayRuntimeResult::Applied;
    }

    storage_.settingsRuntime().apply(baseline_);

    const auto result =
        reader_.reader().handle(
            TypographyDefaultsChanged{
                baseline_.reading_preset,
                baseline_.font_size_px,
                baseline_.line_spacing,
                baseline_.margin_px,
            }
        );

    return toOverlayResult(result);
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::close() {
    auto& app = storage_.appState();

    if (app.screen != Screen::ReaderOverlay) {
        return ReaderOverlayRuntimeResult::Ignored;
    }

    if (app.reader_overlay.mode ==
        ReaderOverlayMode::QuickTypography) {
        const auto restored =
            restoreBaseline();

        if (restored !=
            ReaderOverlayRuntimeResult::Applied) {
            return restored;
        }
    }

    baseline_valid_ = false;
    app.screen = Screen::Reading;

    return ReaderOverlayRuntimeResult::Applied;
}

} // namespace enku
