#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "types.hpp"
#include "input.hpp"
#include "diagnostics.hpp"
#include "boot.hpp"
#include "refresh.hpp"

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

struct BookOpened {
    BookId book_id;
};

struct BookOpenFailed {
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

struct RefreshRequested {
    RefreshRequest request;
};

struct RefreshCompleted {
    std::uint32_t generation{0};
};

struct RefreshFailed {
    std::uint32_t generation{0};
};

using AppEvent = std::variant<
    InputReceived,
    ActionRequested,
    OpenBookRequested,
    BookOpened,
    BookOpenFailed,
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
    BootStable,
    RefreshRequested,
    RefreshCompleted,
    RefreshFailed
>;

} // namespace enku
