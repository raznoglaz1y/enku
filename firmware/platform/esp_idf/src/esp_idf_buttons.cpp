#include "enku/platform/esp_idf/esp_idf_buttons.hpp"

#include "driver/gpio.h"

#include "enku/platform/esp_idf/board.hpp"

namespace enku::platform::esp_idf {

EspIdfButtons::EspIdfButtons()
    : buttons_{{
          {
              PhysicalControl::Up,
              static_cast<int>(board::kButtonUp),
          },
          {
              PhysicalControl::Function,
              static_cast<int>(board::kButtonFunction),
          },
          {
              PhysicalControl::Down,
              static_cast<int>(board::kButtonDown),
          },
          {
              PhysicalControl::Boot,
              static_cast<int>(board::kButtonBoot),
          },
      }} {}

bool EspIdfButtons::begin() {
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask =
        (1ULL << board::kButtonUp) |
        (1ULL << board::kButtonFunction) |
        (1ULL << board::kButtonDown) |
        (1ULL << board::kButtonBoot);
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.pull_up_en = GPIO_PULLUP_ENABLE;

    return gpio_config(&config) == ESP_OK;
}

std::optional<PhysicalInputEvent>
EspIdfButtons::update(
    ButtonState& button,
    bool pressed,
    std::uint32_t now_ms
) {
    if (pressed != button.raw_pressed) {
        button.raw_pressed = pressed;
        button.raw_changed_ms = now_ms;
        return std::nullopt;
    }

    if (button.stable_pressed != button.raw_pressed) {
        if (now_ms - button.raw_changed_ms < kDebounceMs) {
            return std::nullopt;
        }

        button.stable_pressed = button.raw_pressed;

        if (button.stable_pressed) {
            button.pressed_ms = now_ms;
            button.last_repeat_ms = now_ms;
            button.long_sent = false;

            return PhysicalInputEvent{
                button.control,
                PressType::Press,
            };
        }

        const bool was_long = button.long_sent;
        button.long_sent = false;

        if (!was_long) {
            return PhysicalInputEvent{
                button.control,
                PressType::Click,
            };
        }

        return PhysicalInputEvent{
            button.control,
            PressType::Release,
        };
    }

    if (!button.stable_pressed) {
        return std::nullopt;
    }

    const auto held_ms = now_ms - button.pressed_ms;

    if (!button.long_sent &&
        held_ms >= kLongPressMs) {
        button.long_sent = true;
        button.last_repeat_ms = now_ms;

        return PhysicalInputEvent{
            button.control,
            PressType::LongPress,
        };
    }

    const bool repeatable =
        button.control == PhysicalControl::Up ||
        button.control == PhysicalControl::Down;

    if (repeatable &&
        held_ms >= kRepeatDelayMs &&
        now_ms - button.last_repeat_ms >=
            kRepeatIntervalMs) {
        button.last_repeat_ms = now_ms;

        return PhysicalInputEvent{
            button.control,
            PressType::Repeat,
        };
    }

    return std::nullopt;
}

std::optional<PhysicalInputEvent>
EspIdfButtons::poll(
    std::uint32_t now_ms
) {
    // Inspect all controls on every 5 ms platform tick. Return at most one
    // event; round-robin starting position keeps simultaneous inputs fair.
    for (std::size_t offset = 0;
         offset < buttons_.size();
         ++offset) {
        const std::size_t index =
            (scan_index_ + offset) %
            buttons_.size();

        auto& button = buttons_[index];

        const bool pressed =
            gpio_get_level(
                static_cast<gpio_num_t>(button.gpio)
            ) == 0;

        const auto event =
            update(button, pressed, now_ms);

        if (event.has_value()) {
            scan_index_ =
                (index + 1U) % buttons_.size();
            return event;
        }
    }

    return std::nullopt;
}

} // namespace enku::platform::esp_idf
