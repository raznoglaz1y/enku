#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "types.hpp"

namespace enku {

struct ButtonPressed {
    std::uint16_t action{0};
};

struct ButtonReleased {
    std::uint16_t action{0};
};

struct ButtonHeld {
    std::uint16_t action{0};
};

struct OpenBookRequested {
    BookId book_id;
};

struct PageNextRequested {};
struct PagePreviousRequested {};
struct BackRequested {};
struct OpenReaderMenuRequested {};
struct OpenSearchRequested {};
struct SleepRequested {};
struct WakeRequested {};

struct OrientationChanged {
    Orientation orientation{Orientation::Portrait};
};

struct BookPositionChanged {
    SemanticPosition position;
};

struct ImportProgressChanged {
    std::uint8_t percent{0};
};

struct ImportCompleted {
    BookId book_id;
};

struct WiFiConnected {
    std::string ssid;
};

struct WiFiDisconnected {};

struct BatteryStateChanged {
    std::uint8_t percent{0};
    bool charging{false};
};

using AppEvent = std::variant<
    ButtonPressed,
    ButtonReleased,
    ButtonHeld,
    OpenBookRequested,
    PageNextRequested,
    PagePreviousRequested,
    BackRequested,
    OpenReaderMenuRequested,
    OpenSearchRequested,
    SleepRequested,
    WakeRequested,
    OrientationChanged,
    BookPositionChanged,
    ImportProgressChanged,
    ImportCompleted,
    WiFiConnected,
    WiFiDisconnected,
    BatteryStateChanged
>;

} // namespace enku
