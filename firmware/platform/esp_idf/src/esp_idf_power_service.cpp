#include "enku/platform/esp_idf/esp_idf_power_service.hpp"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "enku/platform/esp_idf/board.hpp"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_PWR";
constexpr std::uint32_t kI2cFrequencyHz = 400000;

constexpr std::uint8_t kStatus1 = 0x00;
constexpr std::uint8_t kStatus2 = 0x01;
constexpr std::uint8_t kCommonConfig = 0x10;
constexpr std::uint8_t kPowerKeyTiming = 0x27;
constexpr std::uint8_t kAdcChannelCtrl = 0x30;
constexpr std::uint8_t kBattVoltageHigh = 0x34;
constexpr std::uint8_t kBattVoltageLow = 0x35;
constexpr std::uint8_t kBattDetectCtrl = 0x68;
constexpr std::uint8_t kBatteryPercent = 0xA4;

} // namespace

EspIdfPowerService::~EspIdfPowerService() {
    if (pmu_ != nullptr) {
        i2c_master_bus_rm_device(pmu_);
        pmu_ = nullptr;
    }

    if (bus_ != nullptr) {
        i2c_del_master_bus(bus_);
        bus_ = nullptr;
    }
}

bool EspIdfPowerService::begin() {
    i2c_master_bus_config_t bus_config = {};
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.scl_io_num = board::kI2cScl;
    bus_config.sda_io_num = board::kI2cSda;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    auto result =
        i2c_new_master_bus(
            &bus_config,
            &bus_
        );

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "I2C bus init failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    i2c_device_config_t device_config = {};
    device_config.dev_addr_length =
        I2C_ADDR_BIT_LEN_7;
    device_config.device_address =
        board::kAxp2101Address;
    device_config.scl_speed_hz =
        kI2cFrequencyHz;

    result =
        i2c_master_bus_add_device(
            bus_,
            &device_config,
            &pmu_
        );

    if (result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "AXP2101 add-device failed: %s",
            esp_err_to_name(result)
        );
        return false;
    }

    // Vendor reference:
    // - battery detection enabled;
    // - battery-voltage ADC enabled;
    // - PWR on = 1 s (enum value 2);
    // - hardware PWR off = 4 s (enum value 0).
    if (!updateBits(kBattDetectCtrl, 0x01, 0x01) ||
        !updateBits(kAdcChannelCtrl, 0x01, 0x01) ||
        !updateBits(kPowerKeyTiming, 0x03, 0x02) ||
        !updateBits(kPowerKeyTiming, 0x0C, 0x00)) {
        ESP_LOGE(kTag, "AXP2101 setup failed");
        return false;
    }

    std::uint8_t status = 0;
    if (!readRegister(kStatus1, status)) {
        ESP_LOGE(
            kTag,
            "AXP2101 did not respond at 0x%02X",
            board::kAxp2101Address
        );
        return false;
    }

    ready_ = true;

    ESP_LOGI(
        kTag,
        "AXP2101 ready on SDA=%d SCL=%d",
        static_cast<int>(board::kI2cSda),
        static_cast<int>(board::kI2cScl)
    );

    return true;
}

bool EspIdfPowerService::readRegister(
    std::uint8_t reg,
    std::uint8_t& value
) const {
    if (pmu_ == nullptr) {
        return false;
    }

    return i2c_master_transmit_receive(
        pmu_,
        &reg,
        1,
        &value,
        1,
        100
    ) == ESP_OK;
}

bool EspIdfPowerService::writeRegister(
    std::uint8_t reg,
    std::uint8_t value
) const {
    if (pmu_ == nullptr) {
        return false;
    }

    const std::uint8_t data[] = {
        reg,
        value,
    };

    return i2c_master_transmit(
        pmu_,
        data,
        sizeof(data),
        100
    ) == ESP_OK;
}

bool EspIdfPowerService::updateBits(
    std::uint8_t reg,
    std::uint8_t mask,
    std::uint8_t value
) const {
    std::uint8_t current = 0;
    if (!readRegister(reg, current)) {
        return false;
    }

    current =
        static_cast<std::uint8_t>(
            (current & ~mask) |
            (value & mask)
        );

    return writeRegister(reg, current);
}

BatteryState EspIdfPowerService::batteryState() const {
    BatteryState state;

    if (!ready_) {
        return state;
    }

    std::uint8_t status1 = 0;
    std::uint8_t status2 = 0;
    std::uint8_t percent = 0;

    if (!readRegister(kStatus1, status1) ||
        !readRegister(kStatus2, status2)) {
        return state;
    }

    const bool battery_connected =
        (status1 & (1U << 3U)) != 0;

    if (battery_connected &&
        readRegister(kBatteryPercent, percent)) {
        state.percent =
            percent <= 100U ? percent : 0U;
    }

    state.charging =
        (status2 >> 5U) == 0x01U;

    const bool vbus_good =
        (status1 & (1U << 5U)) != 0;
    const bool vbus_in =
        (status2 & (1U << 3U)) == 0;

    state.external_power =
        vbus_good && vbus_in;

    return state;
}

std::uint16_t
EspIdfPowerService::batteryVoltageMv() const {
    if (!ready_) {
        return 0;
    }

    std::uint8_t status1 = 0;
    if (!readRegister(kStatus1, status1) ||
        (status1 & (1U << 3U)) == 0) {
        return 0;
    }

    std::uint8_t high = 0;
    std::uint8_t low = 0;

    if (!readRegister(kBattVoltageHigh, high) ||
        !readRegister(kBattVoltageLow, low)) {
        return 0;
    }

    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(
            high & 0x1FU
        ) << 8U) |
        low
    );
}

bool EspIdfPowerService::vbusPresent() const {
    return batteryState().external_power;
}

bool EspIdfPowerService::canSuspend() const {
    return ready_;
}

DevicePowerState
EspIdfPowerService::powerState() const {
    return state_;
}

WakeReason EspIdfPowerService::wakeReason() const {
    return wake_reason_;
}

bool EspIdfPowerService::requestSuspend() {
    if (!ready_) {
        return false;
    }

    const gpio_num_t wake_pins[] = {
        board::kButtonUp,
        board::kButtonFunction,
        board::kButtonDown,
        board::kButtonBoot,
    };

    for (const auto pin : wake_pins) {
        if (gpio_wakeup_enable(
                pin,
                GPIO_INTR_LOW_LEVEL
            ) != ESP_OK) {
            return false;
        }
    }

    if (esp_sleep_enable_gpio_wakeup() != ESP_OK) {
        return false;
    }

    state_ = DevicePowerState::Suspended;
    wake_reason_ = WakeReason::Unknown;

    const auto result = esp_light_sleep_start();

    state_ = DevicePowerState::Active;

    if (result != ESP_OK) {
        return false;
    }

    wake_reason_ = WakeReason::NavigationInput;
    return true;
}

bool EspIdfPowerService::requestPowerOff() {
    if (!ready_) {
        ESP_LOGW(
            kTag,
            "Software power-off unavailable: PMU not ready"
        );
        return false;
    }

    ESP_LOGI(
        kTag,
        "Requesting AXP2101 software shutdown"
    );

    state_ = DevicePowerState::PoweredOff;

    // COMMON_CONFIG bit0 is the AXP2101 software shutdown command.
    if (!updateBits(
            kCommonConfig,
            0x01,
            0x01
        )) {
        ESP_LOGE(
            kTag,
            "AXP2101 shutdown command failed"
        );
        state_ = DevicePowerState::Active;
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    return true;
}

} // namespace enku::platform::esp_idf
