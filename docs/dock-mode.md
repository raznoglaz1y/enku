# ENKU Dock and Dock Mode

Status: **planned / hardware and firmware design target**

ENKU Dock is a separate accessory that turns the reader into a useful low-power desk display while it is charging.

## Product idea

When ENKU is placed on the dock, the device detects the dock connection, enters **Dock Mode**, and renders a user-selected e-paper dashboard instead of behaving like an ordinary sleeping reader.

The dock is intentionally optional. ENKU remains a complete reader without it.

## Physical interface

Preferred first revision:

- 4 spring-loaded pogo contacts on the dock;
- matching gold pads on the lower rear of ENKU;
- magnetic/mechanical alignment in the enclosure, not electrical polarity switching;
- USB-C remains available and unchanged.

Proposed contact allocation:

1. GND
2. +5V
3. DOCK_DETECT / accessory ID
4. +5V or GND redundant power contact in R0.1, with option to reuse as accessory ID/data in a later revision

The final pin order must be mechanically asymmetric or otherwise protected against reversed placement.

## Power path

Dock +5 V and USB-C VBUS must never be hard-wired together without protection.

Target architecture:

```text
USB-C 5 V ---------\
                    > protected OR / power mux -> charger / system power path
Dock 5 V ----------/
```

The mainboard should therefore reserve:
- dock input ESD / transient protection;
- reverse-current blocking or ideal-diode/power-mux stage;
- DOCK_DETECT input;
- test pads for charging-current and dock-detect validation.

## Dock detection

Dock Mode must not depend on guessing from charge current alone.

Preferred detection order:
1. dedicated pogo DOCK_DETECT contact;
2. optional resistor-coded accessory ID;
3. charging-source inference only as a fallback.

This lets firmware distinguish:
- USB cable charging;
- ENKU Dock;
- future accessories.

## Dock Mode UI

Initial selectable screens:

- Clock
- Clock + date
- Clock + battery
- Current book cover + reading progress
- Next reading target / daily reading progress
- Calendar-style date card
- Minimal weather card when cached data is available
- Custom text / quote / status card
- User-defined dashboard preset

Because the panel is e-paper, Dock Mode should avoid unnecessary refreshes.

### Clock behavior

For a minute-resolution clock:
- refresh once per minute only when Dock Mode is active;
- prefer partial/region refresh if panel testing supports it reliably;
- periodically force a full refresh to control ghosting;
- suspend Wi-Fi between sync/update windows unless a widget needs fresh data.

A static or low-refresh dashboard should remain the default for maximum panel quality and low power.

## User settings

Proposed settings:

```text
Dock
  Dock Mode                 On / Off
  Default screen            Clock / Book / Dashboard / Custom
  Clock style               Minimal / Date / Battery / Reading
  Refresh interval          1 min / 5 min / 15 min / Manual
  Wi-Fi updates             Off / Scheduled
  Sleep at night            Off / Schedule
  Full refresh interval     Auto / 15 min / 30 min / 60 min
```

## Custom dashboard model

Long-term target: simple configurable blocks rather than arbitrary HTML.

Candidate widgets:
- time
- date
- battery
- charging state
- current book
- reading progress
- daily reading goal
- next calendar item if a supported local source exists
- cached weather
- custom text

The device should store presets locally and work without a cloud account.

## Dock hardware variants

### Dock Mini
- passive charging stand
- pogo contacts only
- USB-C input on dock
- weighted or rubberized base

### Dock Stand
- angled desk stand
- cable hidden through rear/bottom
- portrait-first
- supports 180-degree reader rotation only if pogo geometry allows it

### Smart Dock — later
Possible future accessory with its own MCU or accessory ID. Not required for R0.1.

## Cost target

Dock should remain a low-complexity accessory.

Preferred BOM:
- 4 pogo pins / spring contacts
- magnets or mechanical alignment parts
- USB-C power-only input
- protection / optional ID resistor
- enclosure/base

No MCU is required for the first dock revision.

## Mainboard requirements created by Dock

- rear 4-pad pogo footprint
- dock 5 V input
- protected OR-ing / reverse-current blocking
- dock-detect GPIO
- charging while hard power is OFF
- no interference with Qi coil keepout, antenna keepout or Hall sensor
- enough enclosure clearance for spring-pin compression

## Wireless charging relationship

Qi remains a separate Pro / optional path.

The Dock does **not** require Qi. A pogo dock is cheaper, thinner and easier to validate, while Pro may optionally add a receiver coil for pad-style wireless charging.

## Validation checklist

Before calling Dock Mode production-ready:

- verify reversed/misaligned dock cannot damage ENKU;
- measure contact resistance and temperature at target charge current;
- validate hot-plug behavior;
- verify USB-C and dock simultaneous connection is safe;
- verify hard-OFF charging;
- verify DOCK_DETECT debounce;
- test magnet interaction with Hall sensor and IMU;
- test e-paper clock ghosting over multi-hour operation;
- measure overnight Dock Mode power use.
