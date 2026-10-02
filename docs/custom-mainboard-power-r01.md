# ENKU custom mainboard R0.1 — power architecture

Status: **candidate architecture / not schematic-frozen**

This document starts the next engineering pass after the competitive/BOM review.

## Goal

Support all ENKU variants with one coherent battery/power architecture:

- USB-C charging;
- Dock pogo charging;
- charging while hard power is OFF;
- safe operation while charging;
- future Pro Wireless Qi input;
- low battery-only quiescent current;
- no direct shorting between independent 5 V sources.

## Charger / system power-path candidate

Primary candidate for R0.1 evaluation:

**TI BQ25185**

Why it is currently preferred over the earlier MCP73831 placeholder:

- true power-path management;
- 1-cell Li-Ion/Li-Polymer support;
- up to 1 A charge current;
- system-load support while charging;
- 4 µA typical battery-only quiescent current;
- 3 V to 18 V input operating range;
- input-current limiting / VINDPM;
- battery thermistor support;
- battery/system protection features;
- small WSON package.

Official source:
https://www.ti.com/product/BQ25185

The earlier MCP73831 remains a useful simple charger reference, but it does not provide the power-path behavior now required by USB + Dock + hard-OFF charging.

## Alternative charger candidates

### BQ24074

- active TI part;
- 1.5 A charger;
- PowerPath;
- 4.2 V cell target;
- mature design.

Source:
https://www.ti.com/product/BQ24074

Reason not currently first choice:
- older architecture;
- higher capability than ENKU likely needs;
- BQ25185 offers a newer low-Iq path and wider input range.

### MCP73871

- in production;
- integrated system load sharing;
- ideal-diode style power-path behavior;
- USB / adapter source handling.

Source:
https://www.microchip.com/en-us/product/MCP73871

Reason to retain as fallback:
- mature portable-device architecture;
- useful if BQ25185 sourcing / assembly / package proves undesirable.

## Wired input architecture

Target:

```text
USB-C VBUS ----\
                > protected source select / OR ----> BQ25185 VIN
Dock 5 V ------/
                                              |
                                              +--> SYS --> ENKU system rail
                                              |
                                              +--> BAT --> 1S Li-Po
```

USB-C and Dock must not simply be tied together.

## Dock source selection

Two implementation approaches remain under review.

### A. Low-cost ideal-diode / FET OR-ing

Preferred if:
- leakage is low enough;
- no source-preference behavior is required;
- BOM / area is significantly lower than an integrated power mux.

### B. Integrated power mux

Reference candidate:
**TI TPS2121**

Features:
- dual-input / single-output;
- automatic input selection;
- ideal-diode behavior;
- reverse-current blocking during transfer;
- fast source switchover;
- configurable current limiting.

Source:
https://www.ti.com/product/TPS2121

Caution:
TPS2121 has materially higher quiescent current than ENKU wants on the battery path. It should sit only on external 5 V inputs, ahead of the charger, so it is not powered from the battery in normal reading operation.

The final choice is a cost/leakage/area decision.

## Hard power switch

The hard power switch should disable the **system load path**, not disconnect the battery from the charger.

Required behavior:

```text
Hard OFF:
- ESP32 OFF
- microSD OFF
- EPD system electronics OFF
- frontlight OFF
- Qi/system accessories OFF where possible
- battery remains connected to charger
- USB / Dock charging remains possible
```

The physical switch should control a load-switch / high-side path rather than carry all transient current directly.

## Pro Wireless architecture

For Pro Wireless, use a **wireless receiver that outputs a regulated supply into the common charger path**, rather than a second independent battery charger, unless later testing shows a compelling reason otherwise.

Reference candidate:
**TI BQ51013B / newer BQ51013C family**

BQ51013B reference capabilities:
- Qi-compatible receiver;
- up to 5 W;
- 5 V regulated output;
- integrated rectifier / control.

Source:
https://www.ti.com/product/BQ51013B

Preferred conceptual path:

```text
Qi coil
  |
Qi RX -> 5 V
  |
  +----> external-input OR / mux ----> BQ25185
USB-C -----------------------^
Dock ------------------------^
```

Advantage:
- one battery charger controls all charge sources;
- one SYS power path;
- simpler charge-state behavior;
- less risk of two chargers fighting each other.

The BQ51050B is a valid Qi receiver + direct battery charger, but using a second battery-charger path would complicate wired/wireless coordination. It is therefore not the primary architecture candidate.

## Power rails

Preliminary logical rails:

- VBUS_USB
- VBUS_DOCK
- VBUS_QI (Pro Wireless)
- VIN_CHARGER
- VBAT
- VSYS
- 3V3_SYS
- EPD_HV rails
- FRONTLIGHT rail (Pro)
- optional always-on / wake rail only if required by the final Hall / wake design

## System regulator

The previous AP2112K 600 mA LDO remains only a placeholder.

Before schematic freeze, ENKU needs a regulator decision based on:
- ESP32-S3 Wi-Fi burst current;
- microSD write bursts;
- display logic / EPD support load;
- idle quiescent current;
- dropout at low battery;
- heat.

A buck or buck-boost regulator may be preferable to a simple LDO if efficiency/runtime gains justify cost and area.

## Battery thermistor

Because BQ25185 supports battery temperature monitoring, production battery packs should preferably expose or support a validated NTC strategy.

For very early prototypes, a fixed test network may be acceptable only if clearly documented and not treated as production-safe behavior.

## Dock detect

DOCK_DETECT remains separate from the 5 V charging path.

Firmware should distinguish:
- USB charging;
- Dock charging;
- Qi charging if detectable;
- no external power.

Dock Mode should trigger from explicit Dock identification, not from generic "charging".

## Variant impact

| Block | Base | Cover | Pro | Pro Wireless |
| --- | --- | --- | --- | --- |
| BQ25185 candidate | yes | yes | yes | yes |
| USB-C input | yes | yes | yes | yes |
| Dock input | yes | yes | yes | yes |
| Dock detect | yes | yes | yes | yes |
| Qi RX | DNP | DNP | DNP | yes |
| Frontlight power | DNP | DNP | yes | yes |

## Next verification work

1. price BQ25185 / alternatives at 10 / 25 / 50 / 100;
2. build the actual reference schematic around BQ25185;
3. choose external-source OR/mux implementation;
4. size charge current for the final battery;
5. select 3.3 V regulator;
6. model hard-OFF leakage;
7. integrate official EPD power reference;
8. verify Pro frontlight current;
9. then freeze the low-voltage power sheet.
