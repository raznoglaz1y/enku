# ENKU Mainboard R0.1 — provisional ESP32-S3 pin map

Status: **architecture-frozen enough for schematic capture / verify before PCB freeze**

Target module:
**ESP32-S3-WROOM-1-N16R8**

## Constraints applied

Official Espressif constraints used:

- GPIO0, GPIO3, GPIO45, GPIO46 are strapping pins;
- GPIO19 / GPIO20 are native USB D- / D+;
- GPIO43 / GPIO44 are UART0;
- GPIO39–42 overlap JTAG;
- GPIO35–37 should not be used for ordinary peripherals on N16R8-class modules because octal flash/PSRAM routing consumes these signals;
- GPIO26–32 are flash/PSRAM-domain pins and are not available as normal module pins.

References:
- ESP32-S3 datasheet
- ESP32-S3-WROOM-1 datasheet

## Proposed R0.1 mapping

| Function | GPIO | Notes |
| --- | ---: | --- |
| USB D- | 19 | fixed native USB |
| USB D+ | 20 | fixed native USB |
| SPI SCLK | 12 | shared EPD + microSD |
| SPI MOSI | 11 | shared EPD + microSD |
| SPI MISO | 13 | microSD only |
| microSD CS | 10 | dedicated |
| EPD CS | 14 | dedicated |
| EPD DC | 21 | dedicated |
| EPD RST | 47 | dedicated |
| EPD BUSY | 48 | input |
| Button Previous | 4 | direct GPIO |
| Button Next | 5 | direct GPIO |
| Button Select/Menu | 6 | direct GPIO |
| Button Back | 7 | direct GPIO |
| BMI270 SDA | 8 | I²C |
| BMI270 SCL | 9 | I²C |
| BMI270 INT1 | 18 | RTC-capable GPIO |
| Hall sensor | 17 | RTC-capable GPIO |
| Battery ADC | 1 | ADC1 |
| Dock detect | 2 | ADC1 / digital |
| Charger status | 15 | optional |
| Qi status / PG | 16 | Pro Wireless / optional |
| Frontlight warm PWM | 39 | Pro; JTAG-overlap accepted |
| Frontlight cool PWM | 40 | Pro; JTAG-overlap accepted |
| Frontlight master enable / spare | 41 | Pro; JTAG-overlap accepted |
| Spare / factory option | 38 | general spare |
| UART RX | 44 | factory/debug |
| UART TX | 43 | factory/debug |
| BOOT / recovery | 0 | strapping; service only |
| Reserved strap | 3 | do not load during boot |
| Reserved strap | 45 | do not load during boot |
| Reserved strap | 46 | do not load during boot |
| GPIO35–37 | — | reserved by N16R8 memory topology |

## Why share SPI between EPD and microSD

R0.1 now prefers one physical SPI bus for:
- SCLK;
- MOSI;

with:
- independent CS for EPD and SD;
- MISO only from SD.

Benefits:
- saves GPIO;
- simpler routing;
- keeps ADC1 / RTC pins available for power, Hall and motion;
- no simultaneous EPD/SD transaction is required by the product architecture.

Firmware rule:
- never access SD while EPD CS is active;
- hold both CS signals at defined inactive levels at boot/sleep;
- keep bus frequency device-specific.

If physical testing exposes bus contention or signal-integrity issues, separate buses remain possible using spare/JTAG-overlap pins, but shared SPI is the preferred first schematic.

## Wake-capable functions

Motion and Hall are intentionally assigned to RTC-capable GPIOs so sleep/wake experiments remain possible.

Exact ESP-IDF deep-sleep wake configuration still requires validation with the final low-power mode.

## Frontlight / JTAG trade

GPIO39–41 are deliberately assigned to Pro-only frontlight functions.

Reason:
- ENKU retains native USB Serial/JTAG capability on GPIO19/20 for normal development;
- UART0 remains on 43/44;
- sacrificing external JTAG pins for Pro-only lighting is acceptable if USB-JTAG remains available.

## Button strategy

R0.1 keeps four direct button GPIOs rather than an ADC resistor ladder.

Why:
- deterministic;
- easier wake/debounce;
- easier prototype debugging;
- primary reading controls are high-value UX.

A resistor ladder can be reconsidered only if later pin pressure becomes real.

## Pins not to casually reuse

Do not repurpose without revisiting the full design:
- GPIO0 / 3 / 45 / 46;
- GPIO19 / 20;
- GPIO35–37;
- GPIO43 / 44;
- GPIO39–41 if Pro frontlight remains on the common PCB.

## Schematic gate

Before PCB routing:
- verify exact WROOM-1-N16R8 module ordering code;
- verify module antenna keepout;
- confirm frontlight needs two PWM channels after FPC verification;
- confirm Qi receiver status pin behavior;
- confirm BQ25185 status outputs actually needed by firmware;
- verify chosen microSD mode is SPI, not SDMMC.
