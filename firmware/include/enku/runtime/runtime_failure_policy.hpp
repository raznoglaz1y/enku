#pragma once

#include <cstdint>

namespace enku {

class RuntimeFailurePolicy {
public:
    explicit constexpr RuntimeFailurePolicy(
        std::uint8_t threshold = 3U
    )
        : threshold_(
              threshold == 0U ? 1U : threshold
          ) {}

    constexpr bool recordFailure() {
        if (consecutive_failures_ < threshold_) {
            ++consecutive_failures_;
        }

        return shouldRecover();
    }

    constexpr void recordSuccess() {
        consecutive_failures_ = 0U;
    }

    constexpr bool shouldRecover() const {
        return consecutive_failures_ >= threshold_;
    }

    constexpr std::uint8_t consecutiveFailures() const {
        return consecutive_failures_;
    }

    constexpr std::uint8_t threshold() const {
        return threshold_;
    }

private:
    std::uint8_t threshold_{3U};
    std::uint8_t consecutive_failures_{0U};
};

} // namespace enku
