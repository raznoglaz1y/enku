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

#include "enku/platform/esp_idf/board.hpp"
#include "enku/platform/esp_idf/esp_idf_epaper.hpp"
#include "enku/platform/esp_idf/esp_idf_file_store.hpp"
#include "enku/platform/esp_idf/esp_idf_sd_card.hpp"

namespace {

constexpr const char* kTag = "ENKU";

using enku::platform::esp_idf::EspIdfEpaper;

struct Glyph {
    char character;
    std::array<std::uint8_t, 7> rows;
};

constexpr std::array<Glyph, 21> kGlyphs = {{
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

bool displaySmokeTest() {
    EspIdfEpaper display;

    if (display.begin() !=
        enku::platform::esp_idf::EpaperStatus::Ok) {
        ESP_LOGE(kTag, "E-paper bus init failed");
        return false;
    }

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

        fillRect(framebuffer, 180, 338, 440, 54);
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

    enku::platform::esp_idf::EspIdfSdCard sd_card;
    const auto mount_status = sd_card.mount();

    if (mount_status !=
        enku::platform::esp_idf::SdMountStatus::Ok) {
        ESP_LOGE(
            kTag,
            "TF initialization failed; bring-up stopped"
        );
        return;
    }

    enku::platform::esp_idf::EspIdfFilesystem filesystem(
        enku::platform::esp_idf::board::kSdMountPoint
    );
    enku::platform::esp_idf::EspIdfBookFileStore book_files(
        filesystem
    );
    enku::platform::esp_idf::EspIdfStateFileStore state_files(
        filesystem
    );

    (void)state_files;

    if (!storageSmokeTest(book_files)) {
        ESP_LOGE(
            kTag,
            "Platform storage adapter verification failed"
        );
        return;
    }

    if (!displaySmokeTest()) {
        ESP_LOGE(
            kTag,
            "Platform display verification failed"
        );
        return;
    }

    ESP_LOGI(
        kTag,
        "ENKU storage + display bring-up complete"
    );
}
