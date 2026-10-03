# ENKU R0.1 — EPD integration baseline

Status: **reference-circuit baseline / current-panel pinout verification still required before fabrication**

This document closes most of the conceptual EPD-HV gap without pretending the board is fabrication-ready.

## 1. What is now verified

Both current ENKU display candidates are current Good Display products using SSD1677 and a 24-pin 0.5 mm FPC:

### Base
**GDEY0397T81P**
- 3.97 inch
- 800 × 480
- 235 ppi
- SSD1677
- 24-pin 0.5 mm FPC
- 96.62 × 56.24 × 0.92 mm
- fast refresh 1.5 s
- partial refresh 0.3 s

Official:
https://www.good-display.com/product/613.html

### Pro / Pro Wireless
**GDEY0426T82-FL01C**
- 4.26 inch
- 800 × 480
- 218 ppi
- SSD1677
- 24-pin 0.5 mm EPD FPC
- separate 6-pin 0.5 mm frontlight FPC
- 105.33 × 62.37 × 1.8 mm
- warm + cool frontlight
- frontlight strings in series
- <= 15 V / <= 15 mA per vendor product page

Official:
https://www.good-display.com/product/880.html

The old GDEQ0426T82-FL01C is EOL and is not a new-design target.

## 2. SSD1677 power architecture

The SSD1677 includes the display voltage generator control but **still requires an external application circuit** for the booster/regulator.

The SSD1677 documentation explicitly states that it generates/controls:
- VGHR / VGLR;
- VSH1R / VSH2R / VSLR;
- VCOM;

and requires external components around:
- GDR;
- RESE;
- inductor;
- switching MOSFET;
- diodes;
- HV capacitors.

Therefore ENKU must not treat either raw panel as a logic-only SPI device.

Reference:
https://www.e-paper-display.com/SSD1677Specification.pdf

## 3. 24-pin family pinout baseline

Published GDEQ/GDEY 4.26-inch SSD1677 family documentation gives the following interface:

| Pin | Signal | ENKU use |
| ---: | --- | --- |
| 1 | NC | no connection |
| 2 | GDR | external HV switch gate drive |
| 3 | RESE | current-sense input |
| 4 | NC | no connection |
| 5 | VSH2 | HV decoupling |
| 6 | NC | no connection |
| 7 | NC | no connection |
| 8 | BS1 | tie LOW for 4-wire SPI |
| 9 | BUSY | MCU input |
| 10 | RES# | MCU reset output |
| 11 | D/C# | MCU data/command |
| 12 | CS# | MCU chip select |
| 13 | SCL | SPI clock |
| 14 | SDA | SPI MOSI |
| 15 | VDDIO | 3V3 logic |
| 16 | VCI | 3V3 panel input |
| 17 | VSS | ground |
| 18 | VDD | core decoupling |
| 19 | VPP | OTP supply / required reference connection |
| 20 | VSH1 | HV decoupling |
| 21 | VGH | HV decoupling |
| 22 | VSL | HV decoupling |
| 23 | VGL | HV decoupling |
| 24 | VCOM | HV decoupling |

This pinout is verified for the 4.26-inch SSD1677 family from published panel documentation.

**Blocker:** the current GDEY0397T81P and GDEY0426T82-FL01C manufacturer PDFs must still be checked directly before final connector numbering/orientation is frozen. A shared controller and connector family is not enough to authorize fabrication.

## 4. External HV reference direction

A current open-hardware SSD1677 design and the SSD1677 reference topology agree on the architectural pattern:

- panel pin GDR drives an external N-channel MOSFET;
- RESE senses boost current through a low-ohmic resistor;
- external inductor + MOSFET + Schottky network create the gate/source rails;
- panel-controlled internal regulators then establish the required waveforms;
- HV rail decouplers need high voltage ratings.

ENKU R0.1 should start from the Good Display / SSD1677 reference values, then validate on the actual selected panels.

### Provisional reference values to verify against the current Good Display PDFs

Candidate values for schematic capture, **not yet manufacturing-frozen**:

- RESE current-sense resistor: **2.2 Ω**
- GDR pull-down: **1 MΩ**
- HV stabilizing capacitors: typically **4.7 µF / 50 V** on the high-voltage rails
- additional lower-value decoupling around VDD/VPP per panel reference
- Schottky diodes rated comfortably above the generated rail voltage
- inductor and MOSFET chosen from the panel/SSD1677 reference operating point

These values are supported by current open-hardware implementation notes that trace the same SSD1677 reference circuit, but the **Good Display current-panel reference is authoritative** and must win if it differs.

## 5. Expected voltage domain

SSD1677 documentation shows representative generated rails around:
- VGH ≈ +20 V;
- VGL ≈ -20 V;
- VSH1 ≈ +15 V;
- VSH2 ≈ +5 V;
- VSL ≈ -15 V;
- VCOM around -2 V depending waveform / panel configuration.

These are controller-reference values, not ENKU production setpoints.

Firmware/panel LUT and the selected panel's OTP/reference data determine actual operation.

## 6. ENKU connector strategy

Use a 24-pin 0.5 mm ZIF/FPC connector.

Current candidate class:
- horizontal SMT;
- top/bottom contact selected only after physical FPC contact-side drawing is verified;
- side latches preferred for serviceability.

The old preliminary BOM used Hirose FH12-24S-0.5SH(55), but the exact connector remains **candidate only** until:
- FPC contact side;
- insertion direction;
- enclosure access;
- height;
- price;
are frozen.

## 7. Logic interface

ENKU should use 4-wire SPI:

- BS1 = LOW;
- SCLK;
- MOSI/SDA;
- CS;
- D/C;
- RST;
- BUSY.

Add:
- 22–33 Ω series resistors on SPI/control outputs as a layout-tuning option;
- defined RST state;
- optional CS pull-up footprint;
- EPD logic decoupling close to FPC.

The panel does not need MISO for normal write operation.

## 8. Shared Base / Pro board feasibility

Base and Pro now look **architecturally compatible enough to continue pursuing one core PCB** because:

- both use SSD1677;
- both use 24-pin 0.5 mm EPD FPC;
- both use the same SPI/control signal family;
- both need the same external HV architecture class.

However this remains conditional on direct comparison of:
- current GDEY0397T81P pinout;
- current GDEY0426T82-FL01C pinout;
- required booster component values;
- FPC contact orientation;
- LUT / waveform initialization;
- mechanical FPC location.

Pro adds only the separate frontlight FPC electrically if the EPD pins prove identical.

## 9. Frontlight FPC

For GDEY0426T82-FL01C, Good Display confirms:
- separate 6-pin 0.5 mm FPC;
- warm + cool LED system;
- series-connected LED strings;
- <=15 V and <=15 mA operating limits.

Do not freeze the warm/cool pin assignment until the current FL0426-S01C drawing is inspected directly.

## 10. Fabrication blockers remaining

EPD integration is not fully closed until:

- [ ] current GDEY0397T81P PDF pin table verified;
- [ ] current GDEY0426T82-FL01C PDF pin table verified;
- [ ] current FL0426-S01C 6-pin assignment verified;
- [ ] FPC contact side/orientation verified for both;
- [ ] official current Good Display booster schematic/reference values verified;
- [ ] exact MOSFET / Schottky / inductor selected and voltage/current margins checked;
- [ ] HV capacitor DC-bias derating checked;
- [ ] first prototype rails scoped during refresh;
- [ ] VGH/VGL/VSH/VSL settling validated;
- [ ] ghosting / waveform behavior tested on both panel variants.

Until those are complete, the EPD block is suitable for **schematic development**, not fabrication approval.


## 11. Current-product freeze matrix

Current Good Display product/specification/drawing availability and the remaining fabrication blockers are tracked separately:

[Display freeze matrix R0.1](display-freeze-r01.md)

The KiCad schematic should follow the named-net/block structure defined here:

[Schematic net/block baseline R0.1](schematic-netlist-r01.md)


## 12. EPD-HV component freeze pass

The generic SSD1677 reference values have now been translated into current sourcing candidates, including an active replacement for the obsolete Si1304BDL reference MOSFET.

See:
[EPD-HV components R0.1](epd-hv-components-r01.md)

Key state:
- R_RESE 2.2 Ω electrical value frozen;
- MBR0530 diode type frozen;
- SI1308EDL-T1-GE3 preferred as the R0.1 quality-baseline MOSFET;
- 47 µH inductor value frozen, production MPN still open;
- SSD1677 capacitor electrical classes frozen;
- panel-specific FPC/contact-side verification remains a fabrication blocker.


## 13. R0.1 blocker closure

R0.1 now uses:
- Hirose FH34SRJ-24S-0.5SH(50), whose dual-sided contacts remove top/bottom-contact connector ambiguity;
- TYS5040470M-10 as the fixed 47 µH prototype inductor;
- IRLML6346TRPBF as the preferred roomy prototype EPD boost switch;
- 40 V / 1 A Schottky rectifiers and 50 V-class HV MLCCs for prototype margin.

See:
[R0.1 blocker closure](r01-blocker-closure.md)

Remaining panel continuity checks are first-board validation gates rather than reasons to keep the CAD worktree blocked.
