# ENKU hardware feature set and product variants

Status: **design target / not yet fabrication-frozen**

This document captures the hardware and enclosure decisions that should be supported by the ENKU custom mainboard and firmware.

## Core product principles

ENKU is a portrait-first, read-first e-reader with physical controls. The base mechanical orientation is portrait; landscape remains a software-supported secondary orientation.

The custom mainboard should be designed once and reused across several enclosure/product variants wherever practical. Optional features should therefore use DNP/population options rather than forcing separate PCBs.

## Common R0.1 / R0.x hardware targets

- ESP32-S3-WROOM-1-N16R8
- 3.97-inch 800x480 e-paper display
- microSD
- native USB-C at the **bottom center**
- reserved low-profile magnetic/pogo charging-dock input as an optional secondary 5 V source
- wireless charging support reserved for a future Pro/accessory implementation rather than mandatory Base hardware
- 1S Li-Po
- physical **hard power-off switch at the top-left**
- right-side physical reading controls
- BMI270 6-axis IMU
- Hall sensor footprint for magnetic cover wake/sleep
- reserved frontlight driver + PWM/EN signals for future illuminated variant
- battery voltage measurement
- debug/test pads
- 4-layer PCB
- ESP32 antenna keepout at PCB edge
- EPD HV/analog block based only on validated panel reference design

## Controls and ergonomics

### Right-side reading controls

Preferred physical layout:

1. Previous page
2. Next page
3. Select / Menu
4. Back

Previous/Next are intended to be implemented as a large two-way rocker or two large tact switches under one rocker cap.

Design goals:
- Next should sit closest to the natural resting position of the thumb.
- Select and Back should be smaller and tactually distinct.
- Page-button mapping must be user-remappable.
- Holding Previous/Next may provide accelerated list scrolling.
- Controls remain usable without looking at the device.

### Orientation and handedness

BMI270 enables:
- automatic portrait/landscape detection
- 180-degree portrait flip
- automatic left-hand/right-hand operation
- remembering the last orientation
- optional motion wake
- optional gesture page turning

The UI should support:
- Auto
- Portrait lock
- Landscape lock
- Remember last orientation

When the reader is rotated 180 degrees, logical Next/Previous behavior should follow the new hand position rather than the original physical top/bottom mapping.

## Motion controls

Motion page turn is optional and disabled by default.

Settings target:
- Gesture page turn: Off / On
- Gesture sensitivity: Low / Medium / High
- Wake on pick-up: Off / On
- Auto rotation: Off / On

Initial gesture concept:
- short deliberate left/right flick
- accelerometer wake trigger
- temporary gyro confirmation window
- direction/rebound validation
- cooldown to prevent double page turns
- reject movements associated with ordinary grip adjustment

Physical page controls always remain available; motion control never replaces them.

## Custom mainboard and kit roadmap

The Waveshare board remains the reference platform for early firmware and electrical validation, but ENKU is now also developing a dedicated portrait-first custom mainboard.

Goals for the custom board:
- lower per-unit cost at small production quantities;
- better enclosure fit than a general-purpose development board;
- native placement of the page rocker, hard power switch, BMI270, Hall sensor and future frontlight circuitry;
- a reproducible BOM suitable for community builds and possible assembled kits;
- one PCB with population options for Base / Cover / Pro rather than separate electronics for every enclosure.

Possible future product formats, only after hardware validation:
- **Electronics kit** — assembled mainboard + display + controls/hardware;
- **Base kit** — electronics + battery + printable/finished enclosure hardware;
- **Cover kit** — Base kit plus magnetic folio;
- **Pro kit** — frontlight-populated electronics and matching optical/mechanical parts.

These are a development and commercialization roadmap, not a statement that kits are currently for sale. Final kit contents, pricing, safety/compliance work and fulfillment model must follow prototype validation.

## Charging interfaces

USB-C remains the primary and universal charging/data connector.

The board should also reserve a secondary charging input so the enclosure family can support a dock without redesigning the whole PCB.

Preferred secondary interface:
- **4-contact magnetic pogo/dock connector** on the lower rear or lower edge;
- two contacts may be paralleled for +5 V / GND or the extra contacts can be used for dock detect / accessory ID;
- connector should be keyed/asymmetric so reverse placement cannot swap polarity;
- secondary 5 V input must be isolated/ORed with USB-C VBUS before the charger/power-path stage.

Wireless charging is a **Pro / optional population target**, not a Base requirement:
- reserve coil area and receiver/charger pads or a small receiver-module interface;
- validate coil, ferrite shield, heat and magnet interaction in the final enclosure;
- avoid placing the Qi coil in the ESP32 antenna keepout or directly under sensitive e-paper circuitry;
- do not promise Qi certification until a complete receiver implementation is tested.

Cost rule: Base ENKU should not carry the full BOM/assembly cost of wireless charging. Pogo charging may be a low-cost accessory option; Qi should be DNP or implemented in Pro.

## Hard power architecture

The top-left switch provides a real hardware system-off state.

Target behavior:
- ESP32, microSD and e-paper system rails are disconnected in OFF.
- USB-C charging remains available while the reader is switched OFF.
- Daily operation uses sleep/wake.
- Hard OFF is intended for storage, travel, service and complete shutdown.

## Magnetic cover support

The mainboard should include a low-power Hall sensor footprint and suitable interrupt routing.

Target UX:
- opening magnetic cover wakes ENKU
- closing cover puts ENKU to sleep
- feature may be disabled in Settings
- hard power switch remains independent

The cover magnet location must be defined jointly with the enclosure after Hall sensor placement is frozen.

## Frontlight-ready architecture

The base PCB should reserve:
- frontlight driver footprint
- PWM signal
- EN signal
- LED power rail connection
- connector/test pads for the light guide / LED assembly

Base ENKU ships with these parts DNP.

The illuminated variant may populate them without requiring a fundamentally different mainboard.

Frontlight settings target:
- Brightness
- Quick-menu brightness control
- Auto-off with sleep
- Optional low-brightness night preset

Color-temperature mixing is explicitly out of scope for the first frontlight revision unless testing proves that the extra LEDs, driver channels and optical stack are worthwhile.

## Enclosure/product variants

### 1. ENKU Slim

Purpose: simplest, lightest and cheapest build.

- portrait-first
- no cover attached
- no frontlight
- hard power top-left
- USB-C bottom-center
- right-side page rocker + Select + Back
- BMI270 motion/orientation support
- Hall sensor may remain populated or DNP depending final cost
- thin 3D-printable shell

This is the reference open-source enclosure.

### 2. ENKU Grip

Purpose: maximum one-hand comfort.

- same electronics as Slim
- slightly wider grip area on the control side
- sculpted / chamfered rear grip
- larger rocker cap
- center of mass biased toward the lower-middle part of the device
- reversible 180-degree operation for left-hand use

This enclosure prioritizes reading comfort over visual symmetry.

### 3. ENKU Cover

Purpose: everyday carry and screen protection.

- magnetic folio cover
- Hall-sensor wake/sleep
- magnetic closure
- cover magnet positioned away from IMU and sensitive power/display circuitry
- case geometry protects page controls from accidental presses
- USB-C remains accessible with cover installed
- optional stand/fold geometry may be explored later

The cover should be removable/replaceable and should not require a different PCB.

### 4. ENKU Pro

Purpose: premium reading variant while preserving the same software/platform.

Includes:
- frontlight optical stack
- populated frontlight driver
- brightness controls
- Hall sensor as standard
- Cover compatibility as standard
- BMI270 motion/orientation controls
- same physical page-button philosophy

The main engineering difference from base ENKU is the display/frontlight stack and populated lighting circuit, not the compute platform.

## Mechanical priorities shared by all variants

- portrait is the primary orientation
- screen centered visually rather than forcing chassis symmetry
- right-side control/grip area may be wider than left bezel
- battery placement should keep center of mass near the lower-middle area
- bottom USB-C must not interfere with little-finger support
- hard power switch must be difficult to trigger accidentally
- page controls should be quiet, low-travel and easy to identify by touch
- enclosure should allow display replacement without destroying the PCB
- antenna area must remain free of metal, magnets and dense copper

## PCB population strategy

A single board should ideally support:

| Feature | Base | Cover | Pro |
| --- | --- | --- | --- |
| BMI270 | populated | populated | populated |
| Hall sensor | optional/populated | populated | populated |
| Frontlight driver | DNP | DNP | populated |
| Frontlight connector | populated/test pads | populated/test pads | populated |
| Hard power | populated | populated | populated |
| Right-side buttons | populated | populated | populated |

This keeps the open-source platform coherent and avoids maintaining separate firmware/hardware branches for every case.

## Next hardware steps

1. Rotate the board concept to portrait-first mechanical orientation.
2. Re-place USB-C to bottom center and hard power to top-left.
3. Place page controls on the right-side edge for thumb reach.
4. Place BMI270 close to the mechanically stable center region.
5. Add Hall sensor candidate location and keep magnet keepout in enclosure CAD.
6. Reserve the frontlight driver/connector area as DNP.
7. Keep EPD HV/analog block reserved until the panel reference design is validated.
8. Add a costed 4-pin pogo/dock charging footprint and input-protection/OR-ing reservation.
9. Reserve Pro-only wireless charging coil/receiver geometry without burdening Base BOM.
10. Produce a new portrait-first Gerber placement preview and keep a per-variant BOM/cost model.


## ENKU Dock and Dock Mode

A separate optional ENKU Dock is now part of the hardware roadmap.

The mainboard should reserve a protected 4-contact pogo interface on the lower rear of the reader. Dock presence should be detected explicitly so firmware can distinguish a desk dock from ordinary USB charging.

While docked, ENKU may enter a configurable low-refresh dashboard mode with options such as:
- clock / date / battery;
- current book and reading progress;
- reading-goal dashboard;
- cached weather or local status cards;
- user-defined static text/dashboard presets.

The first dock should stay passive: pogo contacts, alignment magnets/mechanics and USB-C power input. No dock MCU is required.

See [Dock and Dock Mode](dock-mode.md) for the electrical, firmware and validation plan.


## Display family strategy

### Base / Electronics / Cover

Primary display candidate: **Good Display GDEY0397T81P**

- 3.97-inch
- 800 × 480
- approximately 235 ppi
- SSD1677
- no frontlight
- 24-pin 0.5 mm FPC

Source:
https://www.good-display.com/product/613.html

### Pro / Pro Wireless

Primary display candidate: **Good Display GDEY0426T82-FL01C**

- 4.26-inch
- 800 × 480
- approximately 218 ppi
- SSD1677
- bonded warm/cool frontlight
- 24-pin EPD FPC + separate frontlight FPC

Source:
https://www.good-display.com/product/880.html

The older GDEQ0426T82-FL01C is treated as legacy/EOL and should not be the new design target.

Replacement reference:
https://www.good-display.com/product/1208.html

The design goal is to keep one core mainboard for both display families where the verified FPC pinout, HV requirements and mechanics allow it.

## Variant BOM ceilings

Internal design stop-limits:

| Variant | Landed BOM ceiling |
| --- | ---: |
| Electronics Kit | PLN 155 |
| Base | PLN 180 |
| Cover | PLN 200 |
| Pro | PLN 230 |
| Pro Wireless | PLN 260 |
| Dock | PLN 40 |

These values are not announced retail prices. They are engineering constraints used before final routing.

See:
- [Competitive analysis](competitive-analysis-2026-10.md)
- [Multi-variant BOM architecture](bom-architecture.md)
