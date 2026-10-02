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
    ApplicationReaderRuntime& reader,
    ReaderOverlayRenderer* renderer,
    RefreshService* refresh
)
    : storage_(storage),
      reader_(reader),
      renderer_(renderer),
      refresh_(refresh) {}

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

    return renderOverlay();
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

    return renderOverlay();
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

        return renderOverlay();
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

    return renderOverlay();
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
            return renderOverlay();
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

    return refreshCurrentFrame();
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
ReaderOverlayRuntime::renderOverlay() {
    if (renderer_ != nullptr &&
        !renderer_->renderReaderOverlay(
            storage_.appState()
        )) {
        return ReaderOverlayRuntimeResult::Failed;
    }

    return refreshCurrentFrame();
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::refreshCurrentFrame() {
    if (refresh_ == nullptr) {
        return ReaderOverlayRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::OverlayChanged;
    request.generation = 0;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? ReaderOverlayRuntimeResult::Applied
        : ReaderOverlayRuntimeResult::Failed;
}

ReaderOverlayRuntimeResult
ReaderOverlayRuntime::close() {
    auto& app = storage_.appState();

    if (app.screen != Screen::ReaderOverlay) {
        return ReaderOverlayRuntimeResult::Ignored;
    }

    const bool quick_typography =
        app.reader_overlay.mode ==
        ReaderOverlayMode::QuickTypography;

    app.screen = Screen::Reading;

    if (quick_typography) {
        const auto restored =
            restoreBaseline();

        if (restored !=
            ReaderOverlayRuntimeResult::Applied) {
            app.screen = Screen::ReaderOverlay;
            return restored;
        }
    } else {
        const auto current =
            storage_.settingsRuntime().current();

        const auto redraw =
            reader_.reader().handle(
                TypographyDefaultsChanged{
                    current.reading_preset,
                    current.font_size_px,
                    current.line_spacing,
                    current.margin_px,
                }
            );

        if (redraw != ReaderRuntimeResult::Applied) {
            app.screen = Screen::ReaderOverlay;
            return ReaderOverlayRuntimeResult::Failed;
        }
    }

    baseline_valid_ = false;
    return ReaderOverlayRuntimeResult::Applied;
}

} // namespace enku
