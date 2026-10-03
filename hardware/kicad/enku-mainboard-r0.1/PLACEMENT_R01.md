# ENKU Mainboard R0.1 — placement baseline A

Status: **first real component-placement pass / no routing / no ERC or DRC yet**

Board envelope remains 54 × 94 mm, portrait, four layers.

This pass moves the project beyond the empty board shell and fixes the first mechanically-sensitive component regions.

## Placement priorities

1. Keep the ESP32 antenna at the board edge with a full RF keepout.
2. Put the 24-pin e-paper FPC and its HV booster in the shortest practical cluster.
3. Keep the Pro frontlight connector beside the display interface, but electrically separated from the EPD HV switching loop.
4. Keep microSD card insertion accessible from the left enclosure edge.
5. Keep USB-C centered on the bottom edge.
6. Keep Dock pogo contacts on the rear/lower region.
7. Keep the right-side reading controls aligned with the enclosure thumb rail.
8. Preserve a central rear keepout for the optional Qi coil.
9. Do not route switching nodes through the Qi, RF, button or display-flex mechanical zones.

## First fixed placement group — display interface

### J_EPD
**Hirose FH34SRJ-24S-0.5SH(50)**

Reason:
- 24 positions;
- 0.5 mm pitch;
- dual top/bottom contact;
- removes FPC contact-side uncertainty from R0.1.

Placement:
- upper-right / top display-interface zone;
- horizontal insertion toward the display-flex path.

### L_EPD
**Laird TYS5040470M-10**
- 47 µH;
- 5 × 5 mm class.

Placement:
- immediately adjacent to Q_EPD and the charge-pump capacitors;
- keep the high-di/dt loop compact;
- do not place under display-flex bend radius.

### Q_EPD
**IRLML6346TRPBF**
- SOT-23.

Placement:
- adjacent to L_EPD / RESE resistor;
- short GDR trace;
- short source/sense path.

### D_EPD1..3
40 V / 1 A Schottky class.

Placement:
- inside the same HV island;
- minimize loop area;
- keep HV nodes away from ESP32 antenna and user-accessible edges.

## Pro frontlight group

### J_FL
**Hirose FH34SRJ-6S-0.5SH(50)**

Placed near J_EPD so the Pro panel's two flex tails can enter the board in the same mechanical region.

### FL remap
Six raw connector nets enter a small 0 Ω remap field before reaching:
- FL_LED_PLUS;
- FL_WARM_RETURN;
- FL_COOL_RETURN.

This field remains accessible enough for prototype rework.

### U_FL
**TPS923610DRLR**

Place near:
- 10 µH inductor;
- output capacitor;
- 15 Ω current-sense resistor;
- warm/cool selector FETs.

Do not put the frontlight switching loop inside the Qi-coil keepout.

## Lower-edge I/O

### J_USB
Bottom-center.

### J_SD
Lower-left with card insertion through the left enclosure edge.

### Dock pads
Rear/lower center, mechanically separated from USB-C.

## Main compute

ESP32-S3-WROOM-1-N16R8 stays along the left upper/middle side with its antenna toward the board edge.

No copper, planes, battery, flex cable or enclosure metal should violate the module antenna keepout.

## Controls

Right edge:
- PREV;
- NEXT;
- SELECT;
- BACK.

The current PCB coordinates are only electrical-placement targets. Exact switch body positions will be reconciled with the rocker/button mechanical CAD before footprint freeze.

## Power

BQ25185 / TPS2121 / TPS63031 should form a compact lower power island between:
- USB/Dock inputs;
- battery connector;
- system rail.

Keep the buck-boost switching loop away from:
- ESP32 antenna;
- EPD BUSY/SPI;
- ADC battery sense;
- IMU.

## Prototype measurement access

Do not bury:
- VBAT;
- VSYS;
- 3V3_SYS;
- VBUS_USB;
- VBUS_DOCK;
- EPD VGH/VGL/VSH/VSL;
- frontlight output/current sense.

First-board debugability has priority over shaving the last millimetre of PCB area.

## Current KiCad status

The PCB file now carries the first real display/frontlight placement footprints and exact R0.1 part labels.

It is **not routed** and has not been checked by KiCad ERC/DRC in the current environment.

The schematic remains the authoritative next step. Once it is captured, footprints and net assignments must be synchronized from the schematic before routing.
