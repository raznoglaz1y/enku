#pragma once

#include <cstdint>
#include <optional>

#include "localization.hpp"
#include "types.hpp"

namespace enku {

enum class ErrorDomain : std::uint8_t {
    Application,
    Reader,
    Parser,
    Layout,
    Storage,
    Persistence,
    Import,
    Network,
    Display,
    Input,
    Power,
    Localization,
    System,
};

enum class ErrorSeverity : std::uint8_t {
    Info,
    Warning,
    RecoverableError,
    CriticalError,
    FatalError,
};

enum class RecoveryAction : std::uint8_t {
    None,
    Retry,
    Back,
    ReturnToLibrary,
    ReinsertStorage,
    Reconnect,
    RemoveTemporaryData,
    ResetAffectedState,
    Reboot,
    PowerOff,
};

using ErrorCode = std::uint32_t;

struct AppError {
    ErrorDomain domain{ErrorDomain::Application};
    ErrorSeverity severity{ErrorSeverity::RecoverableError};
    ErrorCode code{0};
    StringKey message_key{0};
    RecoveryAction primary_action{RecoveryAction::Back};
    std::optional<BookId> book_id;
    std::int32_t native_code{0};
};

enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

enum class LogCategory : std::uint8_t {
    Core,
    Input,
    Ui,
    Reader,
    Parser,
    Layout,
    Storage,
    Persistence,
    Import,
    Network,
    Display,
    Power,
    Localization,
    Web,
};

} // namespace enku
