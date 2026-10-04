#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "enku/core/diagnostics.hpp"
#include "enku/services/services.hpp"

namespace enku {

struct DiagnosticLogEntry {
    std::uint64_t sequence{0};
    LogLevel level{LogLevel::Info};
    LogCategory category{LogCategory::Core};
    std::uint32_t event_code{0};
    std::string message;
};

class DiagnosticRingLogService final : public LogService {
public:
    static constexpr std::size_t kMaxMessageBytes = 192U;

    explicit DiagnosticRingLogService(
        std::size_t capacity = 64U
    );

    void write(
        LogLevel level,
        LogCategory category,
        std::uint32_t event_code,
        const std::string& message
    ) override;

    std::size_t size() const;
    std::size_t capacity() const;
    std::uint64_t overwrittenCount() const;

    std::vector<DiagnosticLogEntry> snapshot() const;
    void clear();

private:
    std::vector<DiagnosticLogEntry> entries_;
    std::size_t start_{0};
    std::size_t size_{0};
    std::uint64_t next_sequence_{1};
    std::uint64_t overwritten_count_{0};
};

class InMemoryDiagnosticsService final
    : public DiagnosticsService {
public:
    void report(const AppError& error) override;

    std::optional<AppError>
    lastCriticalError() const override;

    bool recoveryModeRequested() const override;

    void clearRecoveryRequest();

private:
    std::optional<AppError> last_critical_error_;
    bool recovery_mode_requested_{false};
};

} // namespace enku
