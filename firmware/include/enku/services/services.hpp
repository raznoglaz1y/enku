#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../core/types.hpp"
#include "../core/power.hpp"
#include "../core/persistence.hpp"
#include "../core/diagnostics.hpp"
#include "../core/refresh.hpp"
#include "../core/library.hpp"

namespace enku {

enum class BookSourceStatus : std::uint8_t {
    Ok,
    Unavailable,
    ReadFailed,
};

class StorageService {
public:
    virtual ~StorageService() = default;

    virtual bool isAvailable() const = 0;
    virtual bool bookExists(const BookId& book_id) const = 0;
};

class BookSourceService {
public:
    virtual ~BookSourceService() = default;

    virtual BookSourceStatus readSource(
        const BookRecord& record,
        std::string& bytes
    ) = 0;
};

class LibraryService {
public:
    virtual ~LibraryService() = default;

    virtual std::optional<BookRecord> get(const BookId& book_id) const = 0;
    virtual LibraryStatus query(const LibraryQuery& query, LibraryPage& page) const = 0;
    virtual std::optional<BookId> findByFingerprint(const std::string& fingerprint) const = 0;
    virtual LibraryStatus updateSummary(
        const BookId& book_id,
        ReadingState reading_state,
        float progress,
        std::uint64_t last_opened_order
    ) = 0;
};

class PersistenceService {
public:
    virtual ~PersistenceService() = default;

    virtual PersistStatus loadLatest(PersistRecordType type) = 0;
    virtual PersistStatus commit(PersistRecordType type) = 0;
};

class ReaderCheckpointService {
public:
    virtual ~ReaderCheckpointService() = default;

    virtual PersistStatus checkpoint(
        const BookId& book_id,
        const SemanticPosition& position,
        float progress,
        ReadingState reading_state
    ) = 0;
};

class RefreshService {
public:
    virtual ~RefreshService() = default;

    virtual bool busy() const = 0;
    virtual bool submit(const RefreshRequest& request) = 0;
    virtual void cancelObsolete(std::uint32_t minimum_generation) = 0;
    virtual RefreshStats stats() const = 0;
};

class NetworkService {
public:
    virtual ~NetworkService() = default;

    virtual bool connected() const = 0;
    virtual void disconnect() = 0;
};

class PowerService {
public:
    virtual ~PowerService() = default;

    virtual BatteryState batteryState() const = 0;
    virtual bool canSuspend() const = 0;
    virtual DevicePowerState powerState() const = 0;
    virtual WakeReason wakeReason() const = 0;

    virtual bool requestSuspend() = 0;
    virtual void requestPowerOff() = 0;
};

class ClockService {
public:
    virtual ~ClockService() = default;

    virtual std::uint64_t unixTime() const = 0;
};

class LogService {
public:
    virtual ~LogService() = default;

    virtual void write(
        LogLevel level,
        LogCategory category,
        std::uint32_t event_code,
        const std::string& message
    ) = 0;
};

class DiagnosticsService {
public:
    virtual ~DiagnosticsService() = default;

    virtual void report(const AppError& error) = 0;
    virtual std::optional<AppError> lastCriticalError() const = 0;
    virtual bool recoveryModeRequested() const = 0;
};

} // namespace enku
