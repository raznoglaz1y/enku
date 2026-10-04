#include "enku/core/diagnostic_ring.hpp"

#include <algorithm>

namespace enku {

DiagnosticRingLogService::DiagnosticRingLogService(
    std::size_t capacity
) {
    const auto bounded_capacity =
        std::max<std::size_t>(capacity, 1U);

    entries_.resize(bounded_capacity);
}

void DiagnosticRingLogService::write(
    LogLevel level,
    LogCategory category,
    std::uint32_t event_code,
    const std::string& message
) {
    const std::string bounded_message(
        message.data(),
        std::min(
            message.size(),
            kMaxMessageBytes
        )
    );

    const auto index =
        (start_ + size_) % entries_.size();

    if (size_ == entries_.size()) {
        entries_[start_] = DiagnosticLogEntry{
            next_sequence_++,
            level,
            category,
            event_code,
            bounded_message,
        };

        start_ =
            (start_ + 1U) % entries_.size();
        ++overwritten_count_;
        return;
    }

    entries_[index] = DiagnosticLogEntry{
        next_sequence_++,
        level,
        category,
        event_code,
        bounded_message,
    };

    ++size_;
}

std::size_t DiagnosticRingLogService::size() const {
    return size_;
}

std::size_t DiagnosticRingLogService::capacity() const {
    return entries_.size();
}

std::uint64_t
DiagnosticRingLogService::overwrittenCount() const {
    return overwritten_count_;
}

std::vector<DiagnosticLogEntry>
DiagnosticRingLogService::snapshot() const {
    std::vector<DiagnosticLogEntry> result;
    result.reserve(size_);

    for (std::size_t offset = 0;
         offset < size_;
         ++offset) {
        const auto index =
            (start_ + offset) % entries_.size();
        result.push_back(entries_[index]);
    }

    return result;
}

void DiagnosticRingLogService::clear() {
    start_ = 0;
    size_ = 0;
    overwritten_count_ = 0;
}

void InMemoryDiagnosticsService::report(
    const AppError& error
) {
    if (error.severity == ErrorSeverity::CriticalError ||
        error.severity == ErrorSeverity::FatalError) {
        last_critical_error_ = error;
    }

    if (error.severity == ErrorSeverity::FatalError) {
        recovery_mode_requested_ = true;
    }
}

std::optional<AppError>
InMemoryDiagnosticsService::lastCriticalError() const {
    return last_critical_error_;
}

bool InMemoryDiagnosticsService::recoveryModeRequested() const {
    return recovery_mode_requested_;
}

void InMemoryDiagnosticsService::clearRecoveryRequest() {
    recovery_mode_requested_ = false;
}

} // namespace enku
