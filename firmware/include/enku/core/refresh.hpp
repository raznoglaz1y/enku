#pragma once

#include <cstdint>
#include <optional>

namespace enku {

struct Rect {
    std::uint16_t x{0};
    std::uint16_t y{0};
    std::uint16_t width{0};
    std::uint16_t height{0};
};

enum class RefreshClass : std::uint8_t {
    None,
    Region,
    Full,
    Deferred,
};

enum class RefreshReason : std::uint8_t {
    Unknown,
    PageTurn,
    FocusChanged,
    OverlayChanged,
    StatusChanged,
    ProgressChanged,
    OrientationChanged,
    LanguageChanged,
    TypographyChanged,
    SleepScreen,
    ErrorRecovery,
    FirstScreen,
    ScreenChanged,
};

struct RefreshRequest {
    RefreshClass refresh_class{RefreshClass::None};
    RefreshReason reason{RefreshReason::Unknown};
    std::uint32_t generation{0};
    std::optional<Rect> dirty_region;
    bool may_coalesce{true};
    bool may_defer{false};
};

struct RefreshStats {
    std::uint32_t total{0};
    std::uint32_t region{0};
    std::uint32_t full{0};
    std::uint32_t escalated_to_full{0};
    std::uint32_t forced_clean_full{0};
    std::uint32_t coalesced{0};
    std::uint32_t dropped_obsolete{0};
    std::uint32_t failures{0};
};

class RefreshGhostingPolicy {
public:
    explicit constexpr RefreshGhostingPolicy(
        std::uint32_t max_non_clean_updates = 5U
    )
        : max_non_clean_updates_(
              max_non_clean_updates == 0U
                  ? 1U
                  : max_non_clean_updates
          ) {}

    constexpr bool shouldForceCleanFull() const {
        return non_clean_updates_ >=
            max_non_clean_updates_;
    }

    constexpr void recordCleanFull() {
        non_clean_updates_ = 0U;
    }

    constexpr void recordNonCleanUpdate() {
        if (non_clean_updates_ <
            max_non_clean_updates_) {
            ++non_clean_updates_;
        }
    }

    constexpr std::uint32_t nonCleanUpdates() const {
        return non_clean_updates_;
    }

    constexpr std::uint32_t limit() const {
        return max_non_clean_updates_;
    }

private:
    std::uint32_t max_non_clean_updates_{5U};
    std::uint32_t non_clean_updates_{0U};
};

} // namespace enku
