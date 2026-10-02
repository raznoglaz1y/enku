#include "enku/platform/esp_idf/epaper_refresh_service.hpp"

#include <vector>

#include "esp_log.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_REFRESH";

} // namespace

EpaperRefreshService::EpaperRefreshService(
    EspIdfEpaper& display,
    MonoFramebufferSource& framebuffer
)
    : display_(display),
      framebuffer_(framebuffer) {}

bool EpaperRefreshService::begin() {
    if (framebuffer_.width() != EspIdfEpaper::kWidth ||
        framebuffer_.height() != EspIdfEpaper::kHeight ||
        framebuffer_.size() != EspIdfEpaper::kMonoBytes) {
        ESP_LOGE(
            kTag,
            "Framebuffer geometry mismatch"
        );
        return false;
    }

    const auto status = display_.begin();

    initialized_ =
        status == EpaperStatus::Ok;

    return initialized_;
}

bool EpaperRefreshService::busy() const {
    return busy_;
}

void EpaperRefreshService::cancelObsolete(
    std::uint32_t minimum_generation
) {
    if (minimum_generation > minimum_generation_) {
        minimum_generation_ = minimum_generation;
    }
}

RefreshStats EpaperRefreshService::stats() const {
    return stats_;
}

bool EpaperRefreshService::shouldEscalateRegion(
    const Rect& region
) const {
    if (region.width == 0 || region.height == 0) {
        return true;
    }

    if (region.x >= EspIdfEpaper::kWidth ||
        region.y >= EspIdfEpaper::kHeight) {
        return true;
    }

    if (static_cast<std::uint32_t>(region.x) +
            region.width >
        EspIdfEpaper::kWidth ||
        static_cast<std::uint32_t>(region.y) +
            region.height >
        EspIdfEpaper::kHeight) {
        return true;
    }

    const std::uint32_t region_area =
        static_cast<std::uint32_t>(region.width) *
        region.height;

    const std::uint32_t full_area =
        static_cast<std::uint32_t>(
            EspIdfEpaper::kWidth
        ) * EspIdfEpaper::kHeight;

    // Large regions are cheaper/safer as a full fast refresh and avoid
    // accumulating partial-window ghosting over most of the panel.
    return region_area > (full_area / 3U);
}

bool EpaperRefreshService::refreshFull(
    const RefreshRequest&
) {
    EpaperStatus status = EpaperStatus::Ok;

    if (!fast_mode_ready_) {
        status = display_.initializeFast();
        fast_mode_ready_ =
            status == EpaperStatus::Ok;
    }

    if (status != EpaperStatus::Ok) {
        return false;
    }

    status = has_base_frame_
        ? display_.fastRefresh(
              framebuffer_.data(),
              framebuffer_.size()
          )
        : display_.fastBaseRefresh(
              framebuffer_.data(),
              framebuffer_.size()
          );

    if (status == EpaperStatus::Ok) {
        has_base_frame_ = true;
        ++stats_.full;
        return true;
    }

    return false;
}

bool EpaperRefreshService::refreshRegion(
    const RefreshRequest& request
) {
    if (!request.dirty_region.has_value() ||
        !has_base_frame_ ||
        shouldEscalateRegion(
            *request.dirty_region
        )) {
        ++stats_.escalated_to_full;
        return refreshFull(request);
    }

    std::vector<std::uint8_t> region_bytes;

    if (!framebuffer_.copyRegion(
            *request.dirty_region,
            region_bytes
        )) {
        ++stats_.escalated_to_full;
        return refreshFull(request);
    }

    const auto& region = *request.dirty_region;

    const auto status =
        display_.partialRefresh(
            region_bytes.data(),
            region_bytes.size(),
            region.x,
            region.y,
            region.width,
            region.height
        );

    if (status == EpaperStatus::Ok) {
        ++stats_.region;
        return true;
    }

    return false;
}

bool EpaperRefreshService::submit(
    const RefreshRequest& request
) {
    ++stats_.total;

    if (!initialized_ ||
        request.refresh_class ==
            RefreshClass::None) {
        ++stats_.failures;
        return false;
    }

    if (request.generation <
        minimum_generation_) {
        ++stats_.dropped_obsolete;
        return true;
    }

    if (request.may_coalesce &&
        request.generation != 0 &&
        request.generation <=
            last_generation_) {
        ++stats_.coalesced;
        return true;
    }

    if (busy_) {
        ++stats_.failures;
        return false;
    }

    busy_ = true;

    bool ok = false;

    switch (request.refresh_class) {
        case RefreshClass::Region:
            ok = refreshRegion(request);
            break;

        case RefreshClass::Full:
        case RefreshClass::Deferred:
            ok = refreshFull(request);
            break;

        case RefreshClass::None:
        default:
            ok = false;
            break;
    }

    busy_ = false;

    if (!ok) {
        ++stats_.failures;
        return false;
    }

    if (request.generation >
        last_generation_) {
        last_generation_ = request.generation;
    }

    return true;
}

} // namespace enku::platform::esp_idf
