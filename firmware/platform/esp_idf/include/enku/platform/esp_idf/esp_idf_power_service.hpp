#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

#include "enku/services/services.hpp"

namespace enku::platform::esp_idf {

class EspIdfPowerService final : public PowerService {
public:
    EspIdfPowerService() = default;
    ~EspIdfPowerService();

    bool begin();

    BatteryState batteryState() const override;
    bool canSuspend() const override;
    DevicePowerState powerState() const override;
    WakeReason wakeReason() const override;

    bool requestSuspend() override;
    void requestPowerOff() override;

    std::uint16_t batteryVoltageMv() const;
    bool vbusPresent() const;

private:
    mutable i2c_master_bus_handle_t bus_{nullptr};
    mutable i2c_master_dev_handle_t pmu_{nullptr};

    DevicePowerState state_{DevicePowerState::Active};
    WakeReason wake_reason_{WakeReason::ColdBoot};
    bool ready_{false};

    bool readRegister(
        std::uint8_t reg,
        std::uint8_t& value
    ) const;

    bool writeRegister(
        std::uint8_t reg,
        std::uint8_t value
    ) const;

    bool updateBits(
        std::uint8_t reg,
        std::uint8_t mask,
        std::uint8_t value
    ) const;
};

} // namespace enku::platform::esp_idf
