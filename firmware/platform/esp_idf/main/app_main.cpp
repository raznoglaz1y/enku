#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "enku/platform/esp_idf/board.hpp"
#include "enku/platform/esp_idf/esp_idf_buttons.hpp"
#include "enku/runtime/input_runtime.hpp"
#include "enku/platform/esp_idf/esp_idf_epaper.hpp"
#include "enku/platform/esp_idf/esp_idf_file_store.hpp"
#include "enku/platform/esp_idf/esp_idf_power_service.hpp"
#include "enku/platform/esp_idf/esp_idf_platform.hpp"
#include "enku/platform/esp_idf/esp_idf_device_runtime.hpp"
#include "enku/platform/esp_idf/esp_idf_sd_card.hpp"

namespace {

constexpr const char* kTag = "ENKU";

using enku::platform::esp_idf::EspIdfEpaper;

struct Glyph {
    char character;
    std::array<std::uint8_t, 7> rows;
};

constexpr std::array<Glyph, 22> kGlyphs = {{
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'D', {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0F,0x10,0x10,0x17,0x11,0x11,0x0F}},
    {'I', {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'N', {0x11,0x19,0x19,0x15,0x13,0x13,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
}};

const Glyph* glyphFor(char character) {
    for (const auto& glyph : kGlyphs) {
        if (glyph.character == character) {
            return &glyph;
        }
    }

    return nullptr;
}

void setBlack(
    std::uint8_t* framebuffer,
    int x,
    int y
) {
    if (x < 0 || y < 0 ||
        x >= EspIdfEpaper::kWidth ||
        y >= EspIdfEpaper::kHeight) {
        return;
    }

    const std::size_t index =
        static_cast<std::size_t>(y) *
            (EspIdfEpaper::kWidth / 8U) +
        static_cast<std::size_t>(x / 8);

    const auto mask =
        static_cast<std::uint8_t>(
            0x80U >> (x & 7)
        );

    framebuffer[index] &=
        static_cast<std::uint8_t>(~mask);
}

void fillRect(
    std::uint8_t* framebuffer,
    int x,
    int y,
    int width,
    int height
) {
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            setBlack(framebuffer, px, py);
        }
    }
}

void drawRect(
    std::uint8_t* framebuffer,
    int x,
    int y,
    int width,
    int height,
    int thickness
) {
    fillRect(framebuffer, x, y, width, thickness);
    fillRect(
        framebuffer,
        x,
        y + height - thickness,
        width,
        thickness
    );
    fillRect(framebuffer, x, y, thickness, height);
    fillRect(
        framebuffer,
        x + width - thickness,
        y,
        thickness,
        height
    );
}

void drawGlyph(
    std::uint8_t* framebuffer,
    int x,
    int y,
    char character,
    int scale
) {
    const auto* glyph = glyphFor(character);
    if (glyph == nullptr) {
        return;
    }

    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if ((glyph->rows[row] &
                 (1U << (4 - column))) == 0) {
                continue;
            }

            fillRect(
                framebuffer,
                x + column * scale,
                y + row * scale,
                scale,
                scale
            );
        }
    }
}

void drawText(
    std::uint8_t* framebuffer,
    int x,
    int y,
    const char* text,
    int scale
) {
    int cursor = x;

    for (const char* p = text; *p != '\0'; ++p) {
        drawGlyph(
            framebuffer,
            cursor,
            y,
            *p,
            scale
        );
        cursor += 6 * scale;
    }
}

bool storageSmokeTest(
    enku::platform::esp_idf::EspIdfBookFileStore& files
) {
    const std::string logical_path =
        "/system/tmp/platform-smoke.txt";
    const std::string payload =
        "ENKU ESP-IDF storage smoke test\n";

    if (files.write(logical_path, payload) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke write failed");
        return false;
    }

    std::string read_back;
    if (files.read(logical_path, read_back) !=
        enku::BookFileStatus::Ok ||
        read_back != payload) {
        ESP_LOGE(kTag, "Smoke read/verify failed");
        return false;
    }

    std::vector<std::string> tmp_files;
    if (files.list("/system/tmp", tmp_files) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke list failed");
        return false;
    }

    const bool found =
        std::find(
            tmp_files.begin(),
            tmp_files.end(),
            logical_path
        ) != tmp_files.end();

    if (!found) {
        ESP_LOGE(kTag, "Smoke file missing from list");
        return false;
    }

    if (files.remove(logical_path) !=
        enku::BookFileStatus::Ok) {
        ESP_LOGE(kTag, "Smoke remove failed");
        return false;
    }

    ESP_LOGI(kTag, "Storage smoke test passed");
    return true;
}

void setRegionBlack(
    std::uint8_t* region,
    int width,
    int height,
    int x,
    int y
) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }

    const std::size_t row_bytes =
        static_cast<std::size_t>((width + 7) / 8);
    const std::size_t index =
        static_cast<std::size_t>(y) * row_bytes +
        static_cast<std::size_t>(x / 8);

    region[index] &=
        static_cast<std::uint8_t>(
            ~(0x80U >> (x & 7))
        );
}

void drawRegionGlyph(
    std::uint8_t* region,
    int width,
    int height,
    int x,
    int y,
    char character,
    int scale
) {
    const auto* glyph = glyphFor(character);
    if (glyph == nullptr) {
        return;
    }

    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if ((glyph->rows[row] &
                 (1U << (4 - column))) == 0) {
                continue;
            }

            for (int py = 0; py < scale; ++py) {
                for (int px = 0; px < scale; ++px) {
                    setRegionBlack(
                        region,
                        width,
                        height,
                        x + column * scale + px,
                        y + row * scale + py
                    );
                }
            }
        }
    }
}

const char* controlName(enku::PhysicalControl control) {
    switch (control) {
        case enku::PhysicalControl::Up: return "Up";
        case enku::PhysicalControl::Function: return "Function";
        case enku::PhysicalControl::Down: return "Down";
        case enku::PhysicalControl::Boot: return "Boot";
        case enku::PhysicalControl::Power: return "Power";
        default: return "Unknown";
    }
}

const char* pressName(enku::PressType press) {
    switch (press) {
        case enku::PressType::Press: return "Press";
        case enku::PressType::Release: return "Release";
        case enku::PressType::Click: return "Click";
        case enku::PressType::LongPress: return "LongPress";
        case enku::PressType::Repeat: return "Repeat";
        default: return "Unknown";
    }
}

const char* actionName(enku::LogicalAction action) {
    switch (action) {
        case enku::LogicalAction::None: return "None";
        case enku::LogicalAction::NavigatePrevious: return "NavigatePrevious";
        case enku::LogicalAction::NavigateNext: return "NavigateNext";
        case enku::LogicalAction::Confirm: return "Confirm";
        case enku::LogicalAction::Back: return "Back";
        case enku::LogicalAction::OpenReaderMenu: return "OpenReaderMenu";
        case enku::LogicalAction::OpenQuickTypography: return "OpenQuickTypography";
        case enku::LogicalAction::PagePrevious: return "PagePrevious";
        case enku::LogicalAction::PageNext: return "PageNext";
        case enku::LogicalAction::Sleep: return "Sleep";
        case enku::LogicalAction::Wake: return "Wake";
        case enku::LogicalAction::PowerOff: return "PowerOff";
        default: return "Unknown";
    }
}

bool powerSmokeTest(
    enku::platform::esp_idf::EspIdfPowerService& power
) {
    const auto battery = power.batteryState();

    ESP_LOGI(
        kTag,
        "POWER battery=%u%% charging=%s external=%s voltage=%umV",
        static_cast<unsigned>(battery.percent),
        battery.charging ? "yes" : "no",
        battery.external_power ? "yes" : "no",
        static_cast<unsigned>(power.batteryVoltageMv())
    );

    ESP_LOGI(
        kTag,
        "Power smoke test passed (shutdown not triggered automatically)"
    );

    return true;
}

bool inputSmokeTest(
    enku::platform::esp_idf::EspIdfButtons& buttons
) {
    enku::AppState app;
    app.screen = enku::Screen::Library;

    ESP_LOGI(
        kTag,
        "Input smoke test: press Up / Function / Down / BOOT for 12 seconds"
    );

    const std::int64_t start_us = esp_timer_get_time();

    while ((esp_timer_get_time() - start_us) <
           12LL * 1000LL * 1000LL) {
        const auto now_ms =
            static_cast<std::uint32_t>(
                esp_timer_get_time() / 1000LL
            );

        const auto input = buttons.poll(now_ms);

        if (input.has_value()) {
            const auto action =
                enku::InputActionMapper::map(
                    app,
                    *input
                );

            ESP_LOGI(
                kTag,
                "INPUT control=%s press=%s action=%s",
                controlName(input->control),
                pressName(input->press),
                action.has_value()
                    ? actionName(*action)
                    : "-"
            );
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                enku::platform::esp_idf::
                    EspIdfButtons::kPollIntervalMs
            )
        );
    }

    ESP_LOGI(kTag, "Input smoke test complete");
    return true;
}

[[noreturn]] void runApplicationLoop(
    enku::platform::esp_idf::EspIdfDeviceRuntime& device
) {
    ESP_LOGI(
        kTag,
        "Entering ENKU application loop"
    );

    while (true) {
        const auto now_ms =
            static_cast<std::uint32_t>(
                esp_timer_get_time() / 1000LL
            );

        const auto result =
            device.pollInput(now_ms);

        if (result ==
            enku::InputDispatchResult::Failed) {
            ESP_LOGE(
                kTag,
                "Input dispatch failed; screen=%u",
                static_cast<unsigned>(
                    device.storage().appState().screen
                )
            );
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                enku::platform::esp_idf::
                    EspIdfButtons::kPollIntervalMs
            )
        );
    }
}

bool showFontRecovery(
    enku::platform::esp_idf::EspIdfPlatform& platform,
    bool missing_font
) {
    auto& framebuffer = platform.framebuffer();
    framebuffer.clearWhite();

    drawRect(
        framebuffer.mutableData(),
        20,
        20,
        EspIdfEpaper::kWidth - 40,
        EspIdfEpaper::kHeight - 40,
        4
    );
    drawText(
        framebuffer.mutableData(),
        250,
        120,
        "ENKU",
        16
    );
    drawText(
        framebuffer.mutableData(),
        205,
        285,
        missing_font
            ? "FONT FAIL"
            : "FONT ERROR",
        missing_font ? 7 : 6
    );
    drawText(
        framebuffer.mutableData(),
        missing_font ? 235 : 275,
        350,
        missing_font
            ? "ADD FONT"
            : "REBOOT",
        5
    );

    enku::RefreshRequest request;
    request.refresh_class = enku::RefreshClass::Full;
    request.reason = enku::RefreshReason::ErrorRecovery;
    request.generation = 1;
    request.may_coalesce = false;
    request.may_defer = false;

    return platform.refresh().submit(request);
}

[[noreturn]] void idleWithoutFont(
    enku::platform::esp_idf::EspIdfPlatform& platform
) {
    ESP_LOGW(
        kTag,
        "Reader font unavailable; showing recovery screen"
    );

    if (!showFontRecovery(platform, true)) {
        ESP_LOGE(
            kTag,
            "Unable to render font recovery screen"
        );
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

[[noreturn]] void idleRecoveryScreen(
    enku::platform::esp_idf::EspIdfPlatform& platform,
    const char* reason
) {
    ESP_LOGW(
        kTag,
        "Recovery screen active: %s; waiting for storage recovery",
        reason
    );

    constexpr TickType_t kRetryDelay =
        pdMS_TO_TICKS(2000);

    while (true) {
        vTaskDelay(kRetryDelay);

        const auto status =
            platform.sdCard().mount();

        if (status !=
            enku::platform::esp_idf::SdMountStatus::Ok) {
            ESP_LOGW(
                kTag,
                "Storage recovery retry failed with status %u",
                static_cast<unsigned>(status)
            );
            continue;
        }

        ESP_LOGI(
            kTag,
            "Storage recovered; restarting into normal boot"
        );

        vTaskDelay(pdMS_TO_TICKS(250));
        esp_restart();
    }
}

bool showStorageRecovery(
    enku::platform::esp_idf::EspIdfPlatform& platform,
    enku::platform::esp_idf::PlatformInitStatus status
) {
    auto& framebuffer = platform.framebuffer();
    framebuffer.clearWhite();

    drawRect(
        framebuffer.mutableData(),
        20,
        20,
        EspIdfEpaper::kWidth - 40,
        EspIdfEpaper::kHeight - 40,
        4
    );
    drawText(
        framebuffer.mutableData(),
        250,
        120,
        "ENKU",
        16
    );
    if (status ==
        enku::platform::esp_idf::PlatformInitStatus::
            SdDirectorySetupFailed) {
        drawText(
            framebuffer.mutableData(),
            170,
            285,
            "SD SETUP FAIL",
            6
        );
        drawText(
            framebuffer.mutableData(),
            235,
            350,
            "REPAIR SD",
            5
        );
    } else {
        drawText(
            framebuffer.mutableData(),
            250,
            285,
            "SD FAIL",
            8
        );
        drawText(
            framebuffer.mutableData(),
            220,
            350,
            "INSERT SD",
            5
        );
    }

    enku::RefreshRequest request;
    request.refresh_class = enku::RefreshClass::Full;
    request.reason = enku::RefreshReason::ErrorRecovery;
    request.generation = 1;
    request.may_coalesce = false;
    request.may_defer = false;

    return platform.refresh().submit(request);
}


bool displaySmokeTest(
    EspIdfEpaper& display
) {
    auto* framebuffer =
        static_cast<std::uint8_t*>(
            heap_caps_malloc(
                EspIdfEpaper::kMonoBytes,
                MALLOC_CAP_SPIRAM |
                    MALLOC_CAP_8BIT
            )
        );

    if (framebuffer == nullptr) {
        ESP_LOGW(
            kTag,
            "PSRAM framebuffer allocation failed; trying regular heap"
        );

        framebuffer =
            static_cast<std::uint8_t*>(
                heap_caps_malloc(
                    EspIdfEpaper::kMonoBytes,
                    MALLOC_CAP_8BIT
                )
            );
    }

    if (framebuffer == nullptr) {
        ESP_LOGE(
            kTag,
            "Unable to allocate %u-byte framebuffer",
            static_cast<unsigned>(
                EspIdfEpaper::kMonoBytes
            )
        );
        return false;
    }

    std::memset(
        framebuffer,
        0xFF,
        EspIdfEpaper::kMonoBytes
    );

    drawRect(
        framebuffer,
        20,
        20,
        EspIdfEpaper::kWidth - 40,
        EspIdfEpaper::kHeight - 40,
        4
    );

    drawText(framebuffer, 226, 126, "ENKU", 18);
    drawText(
        framebuffer,
        205,
        310,
        "DISPLAY BRING-UP",
        4
    );
    drawText(
        framebuffer,
        250,
        350,
        "FULL REFRESH",
        4
    );

    ESP_LOGI(kTag, "Initializing SSD1677 full-refresh mode");

    auto status = display.initializeFull();
    if (status ==
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGI(kTag, "Sending ENKU framebuffer");
        status = display.fullRefresh(
            framebuffer,
            EspIdfEpaper::kMonoBytes
        );
    }

    if (status ==
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGI(kTag, "Full refresh complete");
        vTaskDelay(pdMS_TO_TICKS(1500));

        std::memset(
            framebuffer,
            0xFF,
            EspIdfEpaper::kMonoBytes
        );

        drawRect(
            framebuffer,
            20,
            20,
            EspIdfEpaper::kWidth - 40,
            EspIdfEpaper::kHeight - 40,
            4
        );

        drawText(framebuffer, 226, 126, "ENKU", 18);
        drawText(
            framebuffer,
            205,
            310,
            "DISPLAY BRING-UP",
            4
        );
        drawText(
            framebuffer,
            250,
            350,
            "FAST REFRESH",
            4
        );

        ESP_LOGI(kTag, "Initializing SSD1677 fast-refresh mode");
        status = display.initializeFast();
    }

    if (status ==
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGI(kTag, "Sending fast framebuffer");
        status = display.fastBaseRefresh(
            framebuffer,
            EspIdfEpaper::kMonoBytes
        );
    }

    constexpr std::uint16_t kPartialX = 616;
    constexpr std::uint16_t kPartialY = 70;
    constexpr std::uint16_t kPartialWidth = 96;
    constexpr std::uint16_t kPartialHeight = 96;
    constexpr std::size_t kPartialBytes =
        (kPartialWidth / 8U) * kPartialHeight;

    std::array<std::uint8_t, kPartialBytes> partial = {};

    if (status ==
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGI(kTag, "Starting partial refresh counter");

        for (char digit : {'0', '1', '2'}) {
            partial.fill(0xFF);

            for (int x = 0; x < kPartialWidth; ++x) {
                setRegionBlack(
                    partial.data(),
                    kPartialWidth,
                    kPartialHeight,
                    x,
                    0
                );
                setRegionBlack(
                    partial.data(),
                    kPartialWidth,
                    kPartialHeight,
                    x,
                    kPartialHeight - 1
                );
            }

            for (int y = 0; y < kPartialHeight; ++y) {
                setRegionBlack(
                    partial.data(),
                    kPartialWidth,
                    kPartialHeight,
                    0,
                    y
                );
                setRegionBlack(
                    partial.data(),
                    kPartialWidth,
                    kPartialHeight,
                    kPartialWidth - 1,
                    y
                );
            }

            drawRegionGlyph(
                partial.data(),
                kPartialWidth,
                kPartialHeight,
                31,
                13,
                digit,
                10
            );

            status = display.partialRefresh(
                partial.data(),
                partial.size(),
                kPartialX,
                kPartialY,
                kPartialWidth,
                kPartialHeight
            );

            if (status !=
                enku::platform::esp_idf::EpaperStatus::Ok) {
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(700));
        }
    }

    if (status ==
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGI(kTag, "Full + fast + partial refresh passed");
        status = display.sleep();
    }

    heap_caps_free(framebuffer);

    if (status !=
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGE(
            kTag,
            "E-paper smoke test failed with status %u",
            static_cast<unsigned>(status)
        );
        return false;
    }

    ESP_LOGI(kTag, "E-paper smoke test passed");
    return true;
}

} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(
        kTag,
        "ENKU platform bring-up: ESP-IDF / ESP32-S3"
    );

    ESP_LOGI(
        kTag,
        "Free heap at boot: %lu bytes",
        static_cast<unsigned long>(
            esp_get_free_heap_size()
        )
    );

    enku::platform::esp_idf::EspIdfPlatform platform;

    const auto& board_profile = platform.boardProfile();
    const auto& board_caps = board_profile.capabilities;

    ESP_LOGI(
        kTag,
        "Board profile: %.*s maturity=%u display=%ux%u storage=%u controls=%u",
        static_cast<int>(board_profile.name.size()),
        board_profile.name.data(),
        static_cast<unsigned>(board_profile.maturity),
        static_cast<unsigned>(board_caps.display_width),
        static_cast<unsigned>(board_caps.display_height),
        static_cast<unsigned>(board_caps.storage_bus),
        static_cast<unsigned>(board_caps.reading_controls)
    );

    ESP_LOGI(
        kTag,
        "Capabilities: psram=%s usb=%s hard-off=%s imu=%s hall=%s dock=%s frontlight=%s qi=%s",
        board_caps.has_psram ? "yes" : "no",
        board_caps.has_native_usb ? "yes" : "no",
        board_caps.has_hard_power_switch ? "yes" : "no",
        board_caps.has_imu ? "yes" : "no",
        board_caps.has_hall_sensor ? "yes" : "no",
        board_caps.has_dock_detect ? "yes" : "no",
        board_caps.has_frontlight ? "yes" : "no",
        board_caps.has_wireless_charging ? "yes" : "no"
    );

    const auto platform_status = platform.begin();

    if (platform_status ==
            enku::platform::esp_idf::PlatformInitStatus::
                SdMountFailed ||
        platform_status ==
            enku::platform::esp_idf::PlatformInitStatus::
                SdDirectorySetupFailed) {
        const char* recovery_reason =
            platform_status ==
                enku::platform::esp_idf::PlatformInitStatus::
                    SdDirectorySetupFailed
                ? "storage directory setup failed"
                : "storage mount failed";

        ESP_LOGE(
            kTag,
            "%s; entering visible recovery mode",
            recovery_reason
        );

        if (!showStorageRecovery(
                platform,
                platform_status
            )) {
            ESP_LOGE(
                kTag,
                "Unable to render storage recovery screen"
            );
        }

        idleRecoveryScreen(
            platform,
            recovery_reason
        );
    }

    if (platform_status !=
        enku::platform::esp_idf::PlatformInitStatus::Ok) {
        ESP_LOGE(
            kTag,
            "Platform initialization failed with status %u",
            static_cast<unsigned>(platform_status)
        );
        return;
    }

#if CONFIG_ENKU_BRINGUP_SMOKE_TESTS
    ESP_LOGI(kTag, "Hardware bring-up smoke tests enabled");

    if (!storageSmokeTest(platform.bookFiles())) {
        ESP_LOGE(
            kTag,
            "Platform storage adapter verification failed"
        );
        return;
    }

    if (!displaySmokeTest(platform.display())) {
        ESP_LOGE(
            kTag,
            "Platform display verification failed"
        );
        return;
    }

    if (!powerSmokeTest(platform.power())) {
        ESP_LOGE(
            kTag,
            "Platform power verification failed"
        );
        return;
    }
#endif

    enku::platform::esp_idf::EspIdfDeviceRuntime device(
        platform,
        enku::TypographySettings{18, 1.35F, 24},
        enku::Viewport{
            EspIdfEpaper::kWidth,
            EspIdfEpaper::kHeight,
        }
    );

    const auto device_status = device.begin();

    if (device_status ==
        enku::platform::esp_idf::DeviceRuntimeInitStatus::FontMissing) {
        ESP_LOGW(
            kTag,
            "Application runtime skipped: copy /system/fonts/NotoSans-Regular.ttf to the TF card"
        );

#if CONFIG_ENKU_RAW_INPUT_DIAGNOSTIC
        if (!inputSmokeTest(platform.buttons())) {
            ESP_LOGE(
                kTag,
                "Platform input verification failed"
            );
            return;
        }
#endif

        idleWithoutFont(platform);
    } else if (device_status ==
        enku::platform::esp_idf::DeviceRuntimeInitStatus::FontInitFailed) {
        ESP_LOGE(
            kTag,
            "Reader font exists but FreeType initialization failed"
        );

        if (!showFontRecovery(platform, false)) {
            ESP_LOGE(
                kTag,
                "Unable to render font initialization recovery screen"
            );
        }

        idleRecoveryScreen(
            platform,
            "font initialization failed"
        );
    } else if (device_status !=
        enku::platform::esp_idf::DeviceRuntimeInitStatus::Ok) {
        ESP_LOGE(
            kTag,
            "Application runtime init failed with status %u",
            static_cast<unsigned>(device_status)
        );
        return;
    } else {
        ESP_LOGI(
            kTag,
            "Application Reader runtime initialized; screen=%u",
            static_cast<unsigned>(
                device.storage().appState().screen
            )
        );

        if (device.storage().appState().screen ==
            enku::Screen::Library) {
            const auto library_result =
                device.reader().library().handle(
                    enku::LibraryRefreshRequested{}
                );

            if (library_result !=
                    enku::LibraryRuntimeResult::Applied &&
                library_result !=
                    enku::LibraryRuntimeResult::Empty) {
                ESP_LOGE(
                    kTag,
                    "Initial Library render failed with status %u",
                    static_cast<unsigned>(library_result)
                );
                return;
            }
        }

        ESP_LOGI(
            kTag,
            "ENKU application runtime ready"
        );

        runApplicationLoop(device);
    }
}
