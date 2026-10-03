# ENKU Mainboard R0.1 — placement baseline B

Status: **mechanical placement baseline C / net sync C / discrete schematic captured / no routing / no KiCad ERC or DRC yet**

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
**Laird TYS5040100M-10**
- 10 µH;
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

BQ25185 / TPS2121 / TPS63802 should form a compact lower power island between:
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

The PCB now carries the main R0.1 mechanical placement, not just the display island.

### Placement baseline B coordinates

| Ref / group | PCB target |
| --- | --- |
| U1 ESP32-S3-WROOM-1-N16R8 | x=32.75, y=45.0, rotated 270°, antenna keepout toward left edge |
| U_IMU BMI270 | x=28.0, y=80.0, outside the central Qi keepout |
| U_HALL DRV5032FBDBZR | x=71.0, y=44.5, above the button rail |
| J_EPD | x=58.0, y=28.0 |
| J_FL | x=67.0, y=34.0 |
| J_SD | x=28.7, y=103.2, lower-left edge |
| J_USB | x=47.0, y=110.0, bottom-center |
| J_DOCK | x=47.0, y=99.0, rear copper side |
| U_SRC / U_CHG / U_3V3 | x=43.5 / 48.5 / 54.0, y=91.0 |
| J_BAT | x=66.0, y=93.5 |
| SW_POWER | x=30.5, y=29.0 |
| PREV / NEXT / SELECT / BACK | x=71.2, y=53 / 61 / 69 / 77 |

The EPD HV and frontlight discretes are now also represented inside the upper display power island:
- D1/D2/D3;
- R_RESE;
- VGH/VGL capacitors;
- L_FL;
- C_FL_OUT;
- R_FL_SET;
- Q_FL_WARM / Q_FL_COOL.

The first collision review found and corrected:
- U1 edge overlap with D2/C2 in the EPD island;
- Hall sensor overlap with the frontlight cluster;
- L_EPD clearances around D1/D2.

### Automatic placement gate

`check_pcb_placement.py` now verifies:
- 54 × 94 mm board outline;
- all critical R0.1 placement references;
- key mechanical regions;
- ESP32 orientation;
- right-side button rail;
- no component origins in the central Qi keepout;
- no accidental copper routing before net synchronization;
- balanced KiCad PCB S-expression.

The GitHub hardware workflow runs this together with the schematic structural gate.

### Next gate

Pads on the placement-only footprints are intentionally **not electrically authoritative yet**.

The next engineering step is:
1. replace placement placeholders with final verified footprints where required;
2. synchronize schematic nets / pad numbers into the PCB;
3. run KiCad ERC;
4. only then begin controlled routing and DRC.

The board is therefore **placed enough to evaluate architecture and mechanics**, but it is not a fabrication release.


## TPS63802 power-island revision

R0.1 no longer uses TPS63031 as the intended population.

Current lower power island:
- U_3V3: TPS63802DLAR exact DLA0010A land pattern;
- L_3V3: Murata DFE201612E-R47M=P2, 0.47 µH, rotated to keep L1/L2 loop short;
- input: 10 µF + 100 nF local bypass;
- output: 2 × 22 µF;
- feedback: 511 kΩ / 91 kΩ on the quiet side of U_3V3;
- PG: 100 kΩ pull-up and diagnostic net.

The placement gate rejects reintroduction of TPS63031 or loss of the support network.


## Verified core footprint pass

The first-spin core no longer uses placement-only footprints for:
- U_SRC — TPS2121 RUX0012A, 12 pads;
- U_CHG — BQ25185 DLH0010A, 10 pins + exposed pad 11;
- U_IMU — BMI270 Bosch LGA-14;
- U_FL — TPS923610 DRL0006A;
- J_EPD — FH34SRJ-24S-0.5SH(50), including S1/S2 retention tabs;
- J_FL — FH34SRJ-6S-0.5SH(50), including S1/S2 retention tabs.

Both Hirose retention tabs are grounded and represented in the schematic so a future schematic-to-PCB sync cannot silently remove their net assignment.

A project-local `ENKU.pretty` library plus `fp-lib-table` makes the verified footprints reproducible without depending on the user's globally installed KiCad library version.

Remaining mechanical footprint gates are intentionally separate:
- final side-button MPN/actuator geometry;
- final battery connector/polarity/NTC strategy;
- final dock pogo mating geometry.
These are enclosure-dependent and must be frozen together with the mechanical stack, not guessed in isolation.
