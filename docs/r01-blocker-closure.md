# ENKU R0.1 — blocker closure decisions

Status: **R0.1 schematic/placement may proceed; first-board validation gates remain**

This pass converts the remaining open questions into explicit engineering decisions rather than leaving them as indefinite blockers.

## 1. EPD FPC contact-side blocker — CLOSED BY CONNECTOR CHOICE

Use:

**Hirose FH34SRJ-24S-0.5SH(50)**

Verified properties:
- 24 positions;
- 0.5 mm pitch;
- horizontal insertion;
- ZIF / back-lock;
- **top and bottom contacts**;
- 1.0 mm connector height;
- 0.30 mm FPC thickness.

Because the connector contacts both sides of the FPC, ENKU no longer depends on choosing a single top-contact vs bottom-contact connector variant.

This removes the connector-contact-side risk from R0.1.

The exact **panel signal order** remains based on the SSD1677 24-pin family mapping and must still be continuity-checked against delivered current panels during first-board validation.

Official Hirose:
https://www.hirose.com/product/p/CL0580-1255-6-50

## 2. Frontlight contact-side blocker — CLOSED BY CONNECTOR CHOICE + REMAP NETWORK

Use:

**Hirose FH34SRJ-6S-0.5SH(50)**

Same dual-contact family as the 24-pin connector.

R0.1 adds a small **frontlight remap matrix** between the raw 6-pin FPC connector and the LED driver/selector block.

Raw connector nets:

- FL_RAW1
- FL_RAW2
- FL_RAW3
- FL_RAW4
- FL_RAW5
- FL_RAW6

Functional nets:

- FL_LED_PLUS
- FL_WARM_RETURN
- FL_COOL_RETURN
- FL_NC_A
- FL_NC_B
- FL_NC_C

Implementation:
- 0 Ω / solder-jumper footprints allow the six raw pins to be mapped to the three functional LED nets after the current FL0426-S01C drawing or first sample is physically verified.
- only the required mapping is populated;
- unused crosspoints remain DNP.

This costs very little on a prototype and prevents a documentation/sample pin-order discrepancy from forcing a PCB respin.

The production revision should collapse the matrix after validation.

## 3. EPD-HV inductor blocker — CLOSED FOR R0.1

Select:

**Laird TYS5040470M-10**

Verified distributor data:
- 47 µH;
- 1 A rated current;
- 1.1 A Isat;
- 272 mΩ max DCR;
- 5.0 × 5.0 mm;
- 4.2 mm max height;
- active;
- strong current distributor availability;
- ~PLN 1.03 at 100 pcs at the October 2026 DigiKey snapshot.

Why this over the larger Bourns SRR0604:
- smaller footprint;
- lower DCR;
- substantially higher current margin;
- lower current unit cost;
- already proven sensible in another independently reviewed SSD1677 implementation.

DigiKey:
https://www.digikey.pl/en/products/detail/laird-signal-integrity-products/TYS5040470M-10/4733473

R0.1 land pattern should be based on the manufacturer drawing, not a generic 5 mm inductor guess.

## 4. EPD-HV MOSFET decision

The earlier SI1308EDL candidate remains electrically acceptable, but the R0.1 PCB should reserve a common SOT-23 land if board area allows because sourcing flexibility matters more than saving ~2 mm².

Preferred R0.1 quality option:
**IRLML6346TRPBF**
- 30 V logic-level N-MOSFET;
- SOT-23;
- ample current margin;
- widely used / easy to source.

Compact alternate:
**SI1308EDL-T1-GE3**
- 30 V;
- SC-70/SOT-323 class.

R0.1 preference: **IRLML6346TRPBF** unless placement pressure proves meaningful.

Reason:
- easier hand-probing/rework;
- more sourcing options;
- stronger prototype current/thermal margin;
- insignificant BOM delta.

## 5. EPD-HV rectifiers

Use a higher-margin 40 V / 1 A Schottky footprint/type for R0.1 rather than optimizing to the original 30 V / 0.5 A minimum.

Preferred candidate:
**1N5819HW-7-F** or qualified equivalent
- 40 V;
- 1 A;
- SOD-123.

This gives more margin around the generated rails for negligible cost.

The original MBR0530 remains electrically reference-compatible and can be evaluated for production cost-down.

## 6. HV capacitors

For R0.1 prototypes, raise the main EPD HV capacitor voltage rating from the generic SSD1677 25 V minimum to **50 V** where practical:

- main charge-pump / rail capacitors: 4.7 µF / 50 V / 0805;
- auxiliary rail capacitors: 1 µF / 50 V / 0805;
- X5R/X7R;
- DC-bias curves checked.

Reason:
- prototype margin;
- generated rails approach ±20 V;
- negligible cost impact compared with the display;
- easier to avoid marginal effective capacitance.

Production can cost-down only after scope measurements.

## 7. What remains a validation gate, not a design blocker

The following no longer prevent schematic/PCB work, but they still prevent calling R0.1 production-ready:

- continuity-check delivered Base panel FPC against SSD1677 family pin numbering;
- continuity/check current Pro frontlight pin mapping;
- scope VGH/VGL/VSH/VSL/VCOM on first board;
- verify frontlight polarity and ~13 mA current;
- confirm no boost overshoot / thermal issue;
- verify connector insertion geometry in the enclosure.

## 8. Result

R0.1 may now proceed to:
- real KiCad project structure;
- schematic capture;
- placement;
- routing after ERC and exact footprint checks.

It may **not** proceed directly to PCBWay fabrication until the schematic passes review and manufacturer land patterns are assigned.
