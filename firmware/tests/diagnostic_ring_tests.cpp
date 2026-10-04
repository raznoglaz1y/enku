#include "enku/core/diagnostic_ring.hpp"

#include <cassert>
#include <string>

using namespace enku;

int main() {
    DiagnosticRingLogService log{3U};

    assert(log.capacity() == 3U);
    assert(log.size() == 0U);
    assert(log.overwrittenCount() == 0U);

    log.write(
        LogLevel::Info,
        LogCategory::Core,
        10U,
        "one"
    );
    log.write(
        LogLevel::Warning,
        LogCategory::Storage,
        20U,
        "two"
    );
    log.write(
        LogLevel::Error,
        LogCategory::Reader,
        30U,
        "three"
    );

    {
        const auto snapshot = log.snapshot();
        assert(snapshot.size() == 3U);
        assert(snapshot[0].sequence == 1U);
        assert(snapshot[0].message == "one");
        assert(snapshot[1].sequence == 2U);
        assert(snapshot[1].message == "two");
        assert(snapshot[2].sequence == 3U);
        assert(snapshot[2].message == "three");
    }

    log.write(
        LogLevel::Critical,
        LogCategory::Display,
        40U,
        "four"
    );

    {
        const auto snapshot = log.snapshot();
        assert(snapshot.size() == 3U);
        assert(snapshot[0].sequence == 2U);
        assert(snapshot[0].message == "two");
        assert(snapshot[1].sequence == 3U);
        assert(snapshot[1].message == "three");
        assert(snapshot[2].sequence == 4U);
        assert(snapshot[2].message == "four");
        assert(log.overwrittenCount() == 1U);
    }

    log.clear();
    assert(log.size() == 0U);
    assert(log.overwrittenCount() == 0U);

    log.write(
        LogLevel::Info,
        LogCategory::Core,
        50U,
        "after-clear"
    );

    {
        const auto snapshot = log.snapshot();
        assert(snapshot.size() == 1U);
        // clear() discards entries but sequence remains monotonic so exported
        // diagnostics can still preserve event ordering across a clear.
        assert(snapshot[0].sequence == 5U);
    }

    DiagnosticRingLogService clamped{0U};
    assert(clamped.capacity() == 1U);

    const std::string oversized(
        DiagnosticRingLogService::kMaxMessageBytes + 64U,
        'x'
    );
    clamped.write(
        LogLevel::Warning,
        LogCategory::Core,
        60U,
        oversized
    );
    {
        const auto snapshot = clamped.snapshot();
        assert(snapshot.size() == 1U);
        assert(
            snapshot[0].message.size() ==
            DiagnosticRingLogService::kMaxMessageBytes
        );
    }

    InMemoryDiagnosticsService diagnostics;

    AppError warning;
    warning.domain = ErrorDomain::Storage;
    warning.severity = ErrorSeverity::Warning;
    warning.code = 100U;
    diagnostics.report(warning);

    assert(!diagnostics.lastCriticalError().has_value());
    assert(!diagnostics.recoveryModeRequested());

    AppError critical;
    critical.domain = ErrorDomain::Display;
    critical.severity = ErrorSeverity::CriticalError;
    critical.code = 200U;
    diagnostics.report(critical);

    assert(diagnostics.lastCriticalError().has_value());
    assert(diagnostics.lastCriticalError()->code == 200U);
    assert(!diagnostics.recoveryModeRequested());

    AppError fatal;
    fatal.domain = ErrorDomain::System;
    fatal.severity = ErrorSeverity::FatalError;
    fatal.code = 300U;
    diagnostics.report(fatal);

    assert(diagnostics.lastCriticalError().has_value());
    assert(diagnostics.lastCriticalError()->code == 300U);
    assert(diagnostics.recoveryModeRequested());

    diagnostics.clearRecoveryRequest();
    assert(!diagnostics.recoveryModeRequested());
    assert(diagnostics.lastCriticalError()->code == 300U);

    return 0;
}
