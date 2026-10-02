#pragma once

#include <cstdint>

#include "driver/gpio.h"

namespace enku::platform::esp_idf::board {

constexpr gpio_num_t kSdClk = GPIO_NUM_16;
constexpr gpio_num_t kSdCmd = GPIO_NUM_17;
constexpr gpio_num_t kSdD0 = GPIO_NUM_15;
constexpr gpio_num_t kSdD1 = GPIO_NUM_7;
constexpr gpio_num_t kSdD2 = GPIO_NUM_8;
constexpr gpio_num_t kSdD3 = GPIO_NUM_18;

constexpr gpio_num_t kI2cSda = GPIO_NUM_41;
constexpr gpio_num_t kI2cScl = GPIO_NUM_42;
constexpr std::uint8_t kAxp2101Address = 0x34;

constexpr gpio_num_t kButtonUp = GPIO_NUM_4;
constexpr gpio_num_t kButtonFunction = GPIO_NUM_5;
constexpr gpio_num_t kButtonDown = GPIO_NUM_6;
constexpr gpio_num_t kButtonBoot = GPIO_NUM_0;

constexpr gpio_num_t kEpdBusy = GPIO_NUM_3;
constexpr gpio_num_t kEpdDc = GPIO_NUM_9;
constexpr gpio_num_t kEpdCs = GPIO_NUM_10;
constexpr gpio_num_t kEpdSclk = GPIO_NUM_11;
constexpr gpio_num_t kEpdMosi = GPIO_NUM_12;
constexpr gpio_num_t kEpdRst = GPIO_NUM_46;

constexpr const char* kSdMountPoint = "/sdcard";

} // namespace enku::platform::esp_idf::board
