#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "../core/types.hpp"
#include "../core/power.hpp"
#include "../core/persistence.hpp"
#include "../core/diagnostics.hpp"
#include "../core/refresh.hpp"
#include "../core/library.hpp"
#include "../core/settings.hpp"

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

    virtual LibraryStatus upsert(const BookRecord& record) = 0;
    virtual LibraryStatus remove(const BookId& book_id) = 0;
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

class SettingsService {
public:
    virtual ~SettingsService() = default;

    virtual PersistStatus load(
        GlobalSettings& settings
    ) = 0;

    virtual PersistStatus save(
        const GlobalSettings& settings
    ) = 0;
};

class PersistenceService {
public:
    virtual ~PersistenceService() = default;

    virtual PersistStatus loadLatest(PersistRecordType type) = 0;
    virtual PersistStatus commit(PersistRecordType type) = 0;
};

struct ReaderCheckpoint {
    SemanticPosition position;
    float progress{0.0F};
    ReadingState reading_state{ReadingState::New};
};

struct AppRestoreContext {
    Screen screen{Screen::Library};
    std::optional<BookId> current_book;
    std::uint32_t library_offset{0};
    std::optional<BookId> library_focused_book;
};

struct BootLoopMarker {
    std::uint8_t incomplete_boot_count{0};
    bool stable{true};
};

class BootLoopService {
public:
    virtual ~BootLoopService() = default;

    virtual PersistStatus beginBoot(
        BootLoopMarker& marker
    ) = 0;

    virtual PersistStatus markStable() = 0;
};

class AppContextService {
public:
    virtual ~AppContextService() = default;

    virtual PersistStatus load(
        AppRestoreContext& context
    ) = 0;

    virtual PersistStatus save(
        const AppRestoreContext& context
    ) = 0;
};

class ReaderCheckpointService {
public:
    virtual ~ReaderCheckpointService() = default;

    virtual PersistStatus load(
        const BookId& book_id,
        ReaderCheckpoint& checkpoint
    ) = 0;

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

enum class NetworkPolicyStatus : std::uint8_t {
    Ok,
    NoTrustedNetwork,
    InvalidCredentials,
    DriverError,
};

class NetworkSettingsService {
public:
    virtual ~NetworkSettingsService() = default;

    virtual NetworkPolicyStatus applyPolicy(
        WiFiPolicy policy
    ) = 0;

    virtual NetworkPolicyStatus setTrustedNetwork(
        std::string_view ssid,
        std::string_view password
    ) = 0;

    virtual NetworkPolicyStatus forgetTrustedNetwork() = 0;

    virtual std::optional<std::string> trustedSsid() const = 0;
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
