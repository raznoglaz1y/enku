#pragma once

#include "driver/gpio.h"

namespace enku::platform::esp_idf::board {

constexpr gpio_num_t kSdClk = GPIO_NUM_43;
constexpr gpio_num_t kSdCmd = GPIO_NUM_44;
constexpr gpio_num_t kSdD0 = GPIO_NUM_39;
constexpr gpio_num_t kSdD1 = GPIO_NUM_40;
constexpr gpio_num_t kSdD2 = GPIO_NUM_41;
constexpr gpio_num_t kSdD3 = GPIO_NUM_42;

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
