#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/spi_master.h"
#include "esp_err.h"

namespace enku::platform::esp_idf {

enum class EpaperStatus : std::uint8_t {
    Ok,
    SpiInitFailed,
    GpioInitFailed,
    TransferFailed,
    BusyTimeout,
};

class EspIdfEpaper {
public:
    static constexpr std::uint16_t kWidth = 800;
    static constexpr std::uint16_t kHeight = 480;
    static constexpr std::size_t kMonoBytes =
        (static_cast<std::size_t>(kWidth) *
         static_cast<std::size_t>(kHeight)) / 8U;

    EspIdfEpaper() = default;
    ~EspIdfEpaper();

    EpaperStatus begin();
    EpaperStatus initializeFull();
    EpaperStatus initializeFast();

    EpaperStatus fullRefresh(
        const std::uint8_t* framebuffer,
        std::size_t size
    );

    EpaperStatus fastRefresh(
        const std::uint8_t* framebuffer,
        std::size_t size
    );

    EpaperStatus fastBaseRefresh(
        const std::uint8_t* framebuffer,
        std::size_t size
    );

    EpaperStatus partialRefresh(
        const std::uint8_t* region,
        std::size_t size,
        std::uint16_t x,
        std::uint16_t y,
        std::uint16_t width,
        std::uint16_t height
    );

    EpaperStatus clearWhite();
    EpaperStatus sleep();

private:
    spi_device_handle_t spi_{nullptr};
    bool bus_initialized_{false};

    EpaperStatus reset();
    EpaperStatus waitReady(
        std::uint32_t timeout_ms = 15000
    );
    EpaperStatus command(std::uint8_t value);
    EpaperStatus data(std::uint8_t value);
    EpaperStatus dataBuffer(
        const std::uint8_t* bytes,
        std::size_t size
    );
    EpaperStatus refreshFull();
    EpaperStatus refreshFast();
    EpaperStatus refreshPartial();
    EpaperStatus transfer(
        bool is_data,
        const std::uint8_t* bytes,
        std::size_t size
    );
};

} // namespace enku::platform::esp_idf
