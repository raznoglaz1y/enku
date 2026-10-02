#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../core/types.hpp"
#include "../core/power.hpp"

namespace enku {

class StorageService {
public:
    virtual ~StorageService() = default;

    virtual bool isAvailable() const = 0;
    virtual bool bookExists(const BookId& book_id) const = 0;
};

class RefreshService {
public:
    virtual ~RefreshService() = default;

    virtual bool busy() const = 0;
    virtual void requestFullRefresh() = 0;
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

    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

} // namespace enku
