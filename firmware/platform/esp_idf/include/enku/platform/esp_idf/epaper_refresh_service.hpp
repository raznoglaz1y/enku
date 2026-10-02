#pragma once

#include <cstdint>

#include "enku/render/mono_framebuffer.hpp"
#include "enku/services/services.hpp"
#include "esp_idf_epaper.hpp"

namespace enku::platform::esp_idf {

class EpaperRefreshService final : public RefreshService {
public:
    EpaperRefreshService(
        EspIdfEpaper& display,
        MonoFramebufferSource& framebuffer
    );

    bool begin();

    bool busy() const override;
    bool submit(
        const RefreshRequest& request
    ) override;

    void cancelObsolete(
        std::uint32_t minimum_generation
    ) override;

    RefreshStats stats() const override;

private:
    EspIdfEpaper& display_;
    MonoFramebufferSource& framebuffer_;

    bool busy_{false};
    bool initialized_{false};
    bool fast_mode_ready_{false};
    bool has_base_frame_{false};

    std::uint32_t minimum_generation_{0};
    std::uint32_t last_generation_{0};
    RefreshStats stats_{};

    bool refreshFull(
        const RefreshRequest& request
    );

    bool refreshRegion(
        const RefreshRequest& request
    );

    bool shouldEscalateRegion(
        const Rect& region
    ) const;
};

} // namespace enku::platform::esp_idf
