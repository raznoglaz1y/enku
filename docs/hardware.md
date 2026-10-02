# ENKU Hardware Baseline

This document tracks the current hardware target and separates **verified facts** from **planned or unverified choices**.

## Reference platform

| Area | Current target | Status |
| --- | --- | --- |
| Main board | Waveshare ESP32-S3-ePaper-3.97 | Selected / ordered |
| MCU | ESP32-S3 | Platform-defined |
| Display | 3.97″ e-paper, 800 × 480 | Platform-defined |
| Input | Physical controls, non-touch UI | Product direction fixed; mapping to verify |
| Storage | microSD / local file workflow | To verify on real board |
| Connectivity | Wi-Fi / BLE capability | Platform-defined; ENKU Wi-Fi behavior planned |
| Battery | ~2000 mAh Li-Po target | Final pack TBD |
| Charging | Board-integrated behavior | To verify |
| Enclosure | Custom 3D-printable case | Starts after measurement |

## What will be verified on arrival

### Board and controls

- exact production revision;
- onboard control type and mapping;
- GPIO behavior where relevant;
- wake-capable inputs;
- connector placement.

### Display

- actual usable refresh modes;
- full-refresh time;
- partial-refresh support and limits;
- ghosting behavior;
- contrast in real reading layouts;
- effect of repeated UI updates.

### Storage

- microSD wiring and interface;
- filesystem support;
- file size / performance limits;
- safe-write behavior during power loss.

### Power

- charging behavior;
- active current;
- reading/idle current;
- sleep current;
- wake behavior;
- practical battery runtime;
- battery voltage/percentage mapping.

## Battery target

The working target is a **single-cell 3.7 V Li-Po around 2000 mAh**, thin enough for a compact reader enclosure.

A previously considered mechanical envelope is approximately **5 × 50 × 60 mm**, but this is not yet a final specification.

Final selection depends on:

- real internal space after board measurement;
- connector type and polarity;
- board charging limits;
- protection circuit;
- thermal/mechanical clearance;
- enclosure thickness target.

No battery model should be treated as final until physically validated.

## Mechanical verification

Before starting the final enclosure, record:

- PCB outline;
- display active area and glass outline;
- total display stack thickness;
- board/display offsets;
- connector protrusions;
- control travel and access;
- screw/mounting features;
- battery keep-out area;
- cable bend radii;
- safe bezel overlap.

The enclosure will be based on these measurements rather than catalog assumptions.

## Planned enclosure variants

The first goal is **one reliable baseline case**.

Only after that is stable, possible variants may include:

- different battery thickness/capacity;
- alternative back shell;
- stand/cover attachment;
- revised button geometry.

The project will avoid fragmenting into many mechanical variants before the baseline design is proven.


## Vendor-confirmed controls

Current official Waveshare source identifies the onboard navigation inputs as active-low GPIOs with pull-ups:

| Control | GPIO | Current vendor naming |
| --- | ---: | --- |
| Up | GPIO4 | Button_Up |
| Function / press | GPIO5 | Button_Function |
| Down | GPIO6 | Button_Down |
| BOOT | GPIO0 | Boot |

The official ESP-IDF example polls these controls with a 5 ms timer and a software button state machine rather than GPIO interrupts.

The PWR key is handled through the AXP2101 PMU path. The current vendor example configures a 1 s power-on press and a 4 s hardware power-off press.

ENKU's logical mappings are documented in [Input & Physical Controls](input-model.md). These source-derived mappings will still be physically verified on the production board when it arrives.


## Vendor-confirmed power behavior

The current official Waveshare application distinguishes panel sleep from full PMU shutdown:

- `EPD_Sleep()` sends the e-paper controller into deep sleep, but the ESP32 application continues running;
- the current full vendor application does not use ESP32 deep sleep as its normal product sleep flow;
- full software power-off calls the AXP2101 shutdown path;
- the vendor PMU setup uses approximately 1 s PWR hold for power-on and 4 s for hardware power-off.

ENKU therefore treats **DisplayIdle**, **Suspended** and **PoweredOff** as separate product states.

The final suspend mechanism and wake sources remain pending real-board measurements. See [Power, Sleep & Wake Model](power-model.md).


## First ESP-IDF bring-up target

ENKU now has an isolated board target under `firmware/platform/esp_idf`.

The first milestone deliberately covers TF storage only.

The board constants currently used by the target are taken from Waveshare's official ESP32-S3-ePaper-3.97 documentation/examples:

### TF / SDMMC

| Signal | GPIO |
| --- | ---: |
| CLK | 43 |
| CMD | 44 |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |

The first target uses 4-bit SDMMC and mounts FAT at `/sdcard`.

### E-paper constants reserved for display bring-up

| Signal | GPIO |
| --- | ---: |
| BUSY | 3 |
| DC | 9 |
| CS | 10 |
| SCLK | 11 |
| MOSI | 12 |
| RST | 46 |

These display pins are now used by the first ENKU SSD1677 driver. The current milestone implements vendor-aligned SPI3 full-refresh initialization, monochrome framebuffer transfer, BUSY timeout handling and panel deep sleep.

The storage adapter implements the same `StateFileStore` and `BookFileStore` contracts already used by the tested host persistence/runtime code. This is the first direct bridge from framework-neutral ENKU storage architecture to the physical Waveshare board.


## SSD1677 full-refresh baseline

The first ENKU panel driver now follows the Waveshare reference full-refresh path:

```text
reset
→ BUSY ready
→ SWRESET
→ SSD1677 full-mode registers
→ 800×480 RAM window
→ write 1-bit framebuffer
→ display update
→ BUSY ready
→ deep sleep
```

The ESP-IDF target uses SPI3 mode 0 at 20 MHz with manual CS, matching the current official Waveshare example.

The driver is compile-verified in GitHub Actions against ESP-IDF v5.5.5. Physical refresh quality, orientation, timing and ghosting still require validation on the delivered board.


## Fast and partial refresh baseline

The platform driver now includes the Waveshare reference refresh modes in addition to full refresh.

Fast mode uses the vendor fast initialization sequence and update control `0xD7`.

Partial mode programs a byte-aligned RAM window, writes only the region buffer, and triggers update control `0xFF`.

The current smoke firmware deliberately exercises all three modes in one boot so physical bring-up can verify:

- full-refresh orientation and mapping;
- fast-refresh latency and ghosting;
- partial-window coordinates;
- repeated partial update artifacts.


## Corrected full-board SD / I2C mapping

The complete official Waveshare firmware for this board uses a different SDMMC pin map from the early/basic e-paper example.

For ENKU board integration we now follow the full board reference:

### SDMMC

| Signal | GPIO |
| --- | ---: |
| CLK | 16 |
| CMD | 17 |
| D0 | 15 |
| D1 | 7 |
| D2 | 8 |
| D3 | 18 |

### Shared board I2C

| Signal | GPIO |
| --- | ---: |
| SDA | 41 |
| SCL | 42 |

The AXP2101 PMU address is `0x34`.

This resolves the apparent GPIO41/42 conflict: it came from mixing the minimal e-paper example's SD map with the complete board firmware's I2C map.

## AXP2101 power baseline

The first ENKU ESP-IDF power service now talks directly to AXP2101 through the native ESP-IDF I2C master driver.

Current capabilities:

- battery percentage;
- battery voltage;
- charging state;
- external/VBUS power presence;
- vendor-aligned PWR on/off timing setup;
- software PMU shutdown;
- light-sleep request with GPIO wake on navigation controls.

The firmware smoke test reads and logs power telemetry but deliberately does **not** call software shutdown automatically.
