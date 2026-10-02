#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "enku/core/input.hpp"

namespace enku::platform::esp_idf {

class EspIdfButtons {
public:
    static constexpr std::uint32_t kPollIntervalMs = 5;
    static constexpr std::uint32_t kDebounceMs = 20;
    static constexpr std::uint32_t kLongPressMs = 650;
    static constexpr std::uint32_t kRepeatDelayMs = 700;
    static constexpr std::uint32_t kRepeatIntervalMs = 180;

    EspIdfButtons();

    bool begin();

    std::optional<PhysicalInputEvent> poll(
        std::uint32_t now_ms
    );

private:
    struct ButtonState {
        PhysicalControl control{PhysicalControl::Function};
        int gpio{0};

        bool raw_pressed{false};
        bool stable_pressed{false};

        std::uint32_t raw_changed_ms{0};
        std::uint32_t pressed_ms{0};
        std::uint32_t last_repeat_ms{0};

        bool long_sent{false};
    };

    std::array<ButtonState, 4> buttons_;
    std::size_t scan_index_{0};

    std::optional<PhysicalInputEvent> update(
        ButtonState& button,
        bool pressed,
        std::uint32_t now_ms
    );
};

} // namespace enku::platform::esp_idf
