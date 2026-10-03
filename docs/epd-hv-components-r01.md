# ENKU R0.1 — EPD-HV component freeze pass

Status: **reference design translated into current sourcing candidates / panel-specific confirmation still required**

## Reference topology

The SSD1677 application circuit explicitly specifies the following external parts:

- C0–C1: 1 µF, 0603, X5R/X7R, 6 V;
- C2–C7: 4.7 µF, 0805, X5R/X7R, 25 V;
- C8: 1 µF, 0805, X7R, 25 V;
- R1: 2.2 Ω, 0805, 1%;
- D1–D3: MBR0530;
- Q1: Si1304BDL;
- L1: 47 µH, CDRH2D18/LDNP-470NC;
- U1: 24-pin 0.5 mm ZIF socket.

The SSD1677 datasheet notes that component values may need adjustment for panel loading.

Reference:
https://files.waveshare.com/upload/2/2a/SSD1677_1.0.pdf

## Production-minded component decisions

### Q1 — replace obsolete Si1304BDL reference with IRLML6346TRPBF

The original Vishay Si1304BDL reference part is obsolete.

Preferred R0.1 quality-baseline replacement:

**Infineon IRLML6346TRPBF**
- 30 V logic-level N-channel MOSFET;
- SOT-23;
- ample prototype current margin;
- broad sourcing and easier first-board probing/rework.

This is also listed as a direct/similar substitute for the obsolete Si1304BDL by distributors.

Why this is preferable to a no-name Si1304 clone:
- exact manufacturer datasheet;
- active sourcing;
- very low RDS(on) compared with the old reference requirement;
- same compact package class;
- small absolute BOM impact.

Current sourcing example:
https://www.lcsc.com/product-detail/C67276.html

Cost-down fallback:
- qualify a lower-cost 30 V logic-level MOSFET only after EPD rail testing;
- do not substitute by footprint/name alone.

### D1–D3 — MBR0530

Keep the SSD1677 reference diode type.

Preferred sourcing baseline:

**YANGJIE MBR0530**
- 30 V;
- 500 mA;
- SOD-123.

Current source:
https://www.lcsc.com/product-detail/schottky-diodes_yangzhou-yangjie-elec-tech-mbr0530_C699108.html

Cost-down alternate:
**CBI MBR0530**
- same nominal 30 V / 500 mA class;
- materially cheaper in volume.

Current source:
https://www.lcsc.com/pl/product-detail/C21714234.html

Rule:
the three EPD-HV diodes must remain an approved common part within one build. Do not mix vendors casually during prototype characterization.

### L1 — 47 µH

R0.1 exact prototype choice:

**Laird TYS5040470M-10**
- 47 µH;
- 1 A rated current;
- 1.1 A saturation current;
- 272 mΩ max DCR;
- 5.0 × 5.0 mm class;
- 4.2 mm max height.

Why:
- significantly more current margin than the original compact SSD1677 reference inductor;
- still fits the current EPD-HV placement zone;
- low enough DCR for a conservative first prototype.

The manufacturer land pattern must be used directly in KiCad.

Production cost-down may qualify a smaller/lower-cost 47 µH part only after VGH/VGL waveform and thermal measurements.

### R_RESE — 2.2 Ω, 1%, 0805

Freeze electrical specification:

- 2.2 Ω;
- 1%;
- 0805;
- ≥0.125 W preferred for margin.

This value comes directly from the SSD1677 reference application circuit.

Manufacturer may remain a qualified passive alternate rather than single-source.

### C2–C7 — 4.7 µF / 25 V / 0805 / X5R or X7R

Reference exact electrical requirement:
- 4.7 µF;
- 0805;
- X5R/X7R;
- 25 V.

Current cost-conscious candidate:
**Walsin 0805X475K250**
- 4.7 µF;
- 25 V;
- X5R;
- ±10%;
- 0805.

Source:
https://www.lcsc.com/product-detail/_4-7uF-475-10-25V_C168908.html

Quality alternate:
**Kyocera AVX 08053D475KAT2A**
- same electrical class.

Source:
https://www.lcsc.com/pl/product-detail/C165207.html

Important:
before production, check vendor DC-bias curves. Nominal 4.7 µF at 25 V is not the same as effective capacitance on a high-voltage EPD rail.

### C8 — 1 µF / 25 V / 0805 / X7R

Candidate:
**FH 0805B105K250AT**
- 1 µF;
- 25 V;
- X7R;
- ±10%;
- 0805.

Source:
https://www.lcsc.com/pl/product-detail/C111769.html

Quality alternate:
**Kyocera AVX 08053C105KAZ2A**
- 1 µF;
- 25 V;
- X7R;
- 0805.

Source:
https://www.lcsc.com/product-detail/C2168801.html

### C0–C1 — 1 µF logic/core decoupling

SSD1677 reference only requires 6 V class for C0–C1.

ENKU should still standardize on:
- 1 µF;
- X7R/X5R;
- 10 V or 16 V;
- 0603;

if the price difference is negligible, reducing passive SKU diversity.

## Cost philosophy

The EPD-HV block is not a good place for aggressive first-pass cost cutting.

The absolute component cost is small relative to:
- the display;
- PCB assembly;
- battery;
- enclosure.

A failed boost rail can:
- damage a display;
- cause ghosting;
- create intermittent refresh failures;
- generate support and warranty costs much larger than a few PLN saved.

Therefore:

1. use the SSD1677 reference electrical values first;
2. use reputable Q1 / L1 parts for R0.1 characterization;
3. verify actual rail waveforms and temperatures;
4. only then qualify cheaper alternates.

## R0.1 exactness state

| Ref | R0.1 state |
| --- | --- |
| R_RESE 2.2 Ω | electrically frozen |
| D1–D3 MBR0530 | type frozen; vendor alternate allowed |
| Q1 IRLML6346TRPBF | preferred prototype MPN |
| L1 TYS5040470M-10 | R0.1 prototype MPN frozen |
| C2–C7 4.7 µF/25 V | electrical spec frozen |
| C8 1 µF/25 V | electrical spec frozen |
| C0–C1 1 µF | electrical spec frozen |
| 24-pin FPC | connector class frozen; contact side still blocked |

## Final blockers before PCBWay upload

- direct verification of current GDEY0397T81P FPC pin numbering/contact orientation;
- direct verification of current GDEY0426T82-FL01C FPC pin numbering/contact orientation;
- direct verification of FL0426-S01C 6-pin assignment;
- confirm actual panel-loading reference values do not override generic SSD1677 values;
- verify effective capacitance under DC bias;
- scope generated rails on first R0.1 board.
