#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "types.hpp"
#include "input.hpp"
#include "diagnostics.hpp"
#include "boot.hpp"

namespace enku {

struct InputReceived {
    PhysicalInputEvent input;
};

struct ActionRequested {
    LogicalAction action{LogicalAction::None};
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

struct ErrorReported {
    AppError error;
};

struct RecoveryModeRequested {};

struct BootStageChanged {
    BootStage stage{BootStage::Reset};
};

struct BootStable {};

using AppEvent = std::variant<
    InputReceived,
    ActionRequested,
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
    BatteryStateChanged,
    ErrorReported,
    RecoveryModeRequested,
    BootStageChanged,
    BootStable
>;

} // namespace enku
