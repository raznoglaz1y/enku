#include "enku/platform/esp_idf/esp_idf_epaper.hpp"

#include <algorithm>
#include <array>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "enku/platform/esp_idf/board.hpp"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_EPD";
constexpr spi_host_device_t kSpiHost = SPI3_HOST;
constexpr int kSpiClockHz = 20 * 1000 * 1000;

} // namespace

EspIdfEpaper::~EspIdfEpaper() {
    if (spi_ != nullptr) {
        spi_bus_remove_device(spi_);
        spi_ = nullptr;
    }

    if (bus_initialized_) {
        spi_bus_free(kSpiHost);
        bus_initialized_ = false;
    }
}

EpaperStatus EspIdfEpaper::begin() {
    gpio_config_t outputs = {};
    outputs.intr_type = GPIO_INTR_DISABLE;
    outputs.mode = GPIO_MODE_OUTPUT;
    outputs.pin_bit_mask =
        (1ULL << board::kEpdRst) |
        (1ULL << board::kEpdDc) |
        (1ULL << board::kEpdCs);
    outputs.pull_down_en = GPIO_PULLDOWN_DISABLE;
    outputs.pull_up_en = GPIO_PULLUP_ENABLE;

    if (gpio_config(&outputs) != ESP_OK) {
        return EpaperStatus::GpioInitFailed;
    }

    gpio_config_t busy = {};
    busy.intr_type = GPIO_INTR_DISABLE;
    busy.mode = GPIO_MODE_INPUT;
    busy.pin_bit_mask = (1ULL << board::kEpdBusy);
    busy.pull_down_en = GPIO_PULLDOWN_ENABLE;
    busy.pull_up_en = GPIO_PULLUP_DISABLE;

    if (gpio_config(&busy) != ESP_OK) {
        return EpaperStatus::GpioInitFailed;
    }

    gpio_set_level(board::kEpdCs, 1);
    gpio_set_level(board::kEpdDc, 1);
    gpio_set_level(board::kEpdRst, 1);

    spi_bus_config_t bus_config = {};
    bus_config.miso_io_num = -1;
    bus_config.mosi_io_num = board::kEpdMosi;
    bus_config.sclk_io_num = board::kEpdSclk;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz = 65536;

    const auto bus_result =
        spi_bus_initialize(
            kSpiHost,
            &bus_config,
            SPI_DMA_CH_AUTO
        );

    if (bus_result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "spi_bus_initialize failed: %s",
            esp_err_to_name(bus_result)
        );
        return EpaperStatus::SpiInitFailed;
    }

    bus_initialized_ = true;

    spi_device_interface_config_t device_config = {};
    device_config.clock_speed_hz = kSpiClockHz;
    device_config.mode = 0;
    device_config.spics_io_num = -1;
    device_config.queue_size = 1;

    const auto device_result =
        spi_bus_add_device(
            kSpiHost,
            &device_config,
            &spi_
        );

    if (device_result != ESP_OK) {
        ESP_LOGE(
            kTag,
            "spi_bus_add_device failed: %s",
            esp_err_to_name(device_result)
        );
        return EpaperStatus::SpiInitFailed;
    }

    ESP_LOGI(
        kTag,
        "SSD1677 bus ready: SPI3, 20 MHz, 800x480"
    );

    return EpaperStatus::Ok;
}

EpaperStatus EspIdfEpaper::transfer(
    bool is_data,
    const std::uint8_t* bytes,
    std::size_t size
) {
    if (spi_ == nullptr || bytes == nullptr || size == 0) {
        return EpaperStatus::TransferFailed;
    }

    gpio_set_level(board::kEpdDc, is_data ? 1 : 0);
    gpio_set_level(board::kEpdCs, 0);

    spi_transaction_t transaction = {};
    transaction.length = size * 8U;
    transaction.tx_buffer = bytes;

    const auto result =
        spi_device_polling_transmit(
            spi_,
            &transaction
        );

    gpio_set_level(board::kEpdCs, 1);

    return result == ESP_OK
        ? EpaperStatus::Ok
        : EpaperStatus::TransferFailed;
}

EpaperStatus EspIdfEpaper::command(
    std::uint8_t value
) {
    return transfer(false, &value, 1);
}

EpaperStatus EspIdfEpaper::data(
    std::uint8_t value
) {
    return transfer(true, &value, 1);
}

EpaperStatus EspIdfEpaper::dataBuffer(
    const std::uint8_t* bytes,
    std::size_t size
) {
    constexpr std::size_t kChunk = 16384;

    std::size_t offset = 0;
    while (offset < size) {
        const auto length =
            std::min(kChunk, size - offset);

        const auto status =
            transfer(
                true,
                bytes + offset,
                length
            );

        if (status != EpaperStatus::Ok) {
            return status;
        }

        offset += length;
    }

    return EpaperStatus::Ok;
}

EpaperStatus EspIdfEpaper::reset() {
    gpio_set_level(board::kEpdRst, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(board::kEpdRst, 0);
    vTaskDelay(pdMS_TO_TICKS(2));

    gpio_set_level(board::kEpdRst, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    return EpaperStatus::Ok;
}

EpaperStatus EspIdfEpaper::waitReady(
    std::uint32_t timeout_ms
) {
    vTaskDelay(pdMS_TO_TICKS(100));

    const std::int64_t start_us =
        esp_timer_get_time();

    while (gpio_get_level(board::kEpdBusy) != 0) {
        const auto elapsed_ms =
            static_cast<std::uint32_t>(
                (esp_timer_get_time() - start_us) /
                1000
            );

        if (elapsed_ms >= timeout_ms) {
            ESP_LOGE(
                kTag,
                "BUSY timeout after %lu ms",
                static_cast<unsigned long>(elapsed_ms)
            );
            return EpaperStatus::BusyTimeout;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return EpaperStatus::Ok;
}

EpaperStatus EspIdfEpaper::initializeFull() {
    if (reset() != EpaperStatus::Ok ||
        waitReady() != EpaperStatus::Ok) {
        return EpaperStatus::BusyTimeout;
    }

    auto send = [&](std::uint8_t cmd,
                    std::initializer_list<std::uint8_t> values)
        -> EpaperStatus {
        if (command(cmd) != EpaperStatus::Ok) {
            return EpaperStatus::TransferFailed;
        }

        for (const auto value : values) {
            if (data(value) != EpaperStatus::Ok) {
                return EpaperStatus::TransferFailed;
            }
        }

        return EpaperStatus::Ok;
    };

    if (command(0x12) != EpaperStatus::Ok ||
        waitReady() != EpaperStatus::Ok ||
        send(0x18, {0x80}) != EpaperStatus::Ok ||
        send(0x0C, {0xAE, 0xC7, 0xC3, 0xC0, 0x80}) !=
            EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    const std::uint16_t height_end = kHeight - 1U;
    const std::uint16_t width_end = kWidth - 1U;

    if (send(
            0x01,
            {
                static_cast<std::uint8_t>(
                    height_end & 0xFFU
                ),
                static_cast<std::uint8_t>(
                    height_end >> 8U
                ),
                0x02,
            }
        ) != EpaperStatus::Ok ||
        send(0x3C, {0x01}) != EpaperStatus::Ok ||
        send(0x11, {0x01}) != EpaperStatus::Ok ||
        send(
            0x44,
            {
                0x00,
                0x00,
                static_cast<std::uint8_t>(
                    width_end & 0xFFU
                ),
                static_cast<std::uint8_t>(
                    width_end >> 8U
                ),
            }
        ) != EpaperStatus::Ok ||
        send(
            0x45,
            {
                static_cast<std::uint8_t>(
                    height_end & 0xFFU
                ),
                static_cast<std::uint8_t>(
                    height_end >> 8U
                ),
                0x00,
                0x00,
            }
        ) != EpaperStatus::Ok ||
        send(0x4E, {0x00, 0x00}) != EpaperStatus::Ok ||
        send(0x4F, {0x00, 0x00}) != EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    return waitReady();
}

EpaperStatus EspIdfEpaper::refreshFull() {
    if (command(0x22) != EpaperStatus::Ok ||
        data(0xF7) != EpaperStatus::Ok ||
        command(0x20) != EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    return waitReady();
}

EpaperStatus EspIdfEpaper::fullRefresh(
    const std::uint8_t* framebuffer,
    std::size_t size
) {
    if (framebuffer == nullptr ||
        size != kMonoBytes) {
        return EpaperStatus::TransferFailed;
    }

    if (command(0x24) != EpaperStatus::Ok ||
        dataBuffer(framebuffer, size) !=
            EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    return refreshFull();
}

EpaperStatus EspIdfEpaper::clearWhite() {
    std::array<std::uint8_t, 1024> white = {};
    white.fill(0xFF);

    if (command(0x24) != EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    std::size_t remaining = kMonoBytes;
    while (remaining > 0) {
        const auto length =
            std::min(remaining, white.size());

        if (dataBuffer(
                white.data(),
                length
            ) != EpaperStatus::Ok) {
            return EpaperStatus::TransferFailed;
        }

        remaining -= length;
    }

    return refreshFull();
}

EpaperStatus EspIdfEpaper::sleep() {
    if (command(0x10) != EpaperStatus::Ok ||
        data(0x01) != EpaperStatus::Ok) {
        return EpaperStatus::TransferFailed;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(board::kEpdRst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    return EpaperStatus::Ok;
}

} // namespace enku::platform::esp_idf
