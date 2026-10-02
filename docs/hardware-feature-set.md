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
8. Produce a new portrait-first Gerber placement preview.
