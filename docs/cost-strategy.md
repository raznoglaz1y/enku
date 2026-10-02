# ENKU cost strategy and kit economics

Status: **planning model — not supplier quotes**

The custom PCB should be designed for manufacturability and kit economics from the start. Cost is a product requirement, not a later optimization.

## Cost rules

1. Prefer one PCB across Base / Cover / Pro variants.
2. Use DNP population options for Hall/frontlight/experimental features where practical.
3. Avoid expensive connectors and sensors unless they materially improve UX.
4. Keep assembly single-sided where practical; move to two-sided only when density forces it.
5. Use common LCSC/JLC/PCBWay-available passives and packages.
6. Maintain at least two sourcing candidates for high-risk parts.
7. Separate open-source reproducibility from premium enclosure/cover options.
8. Customer shipping is modeled separately from reward price.

## Current planning targets

At approximately 50 units, the working cost model currently targets:

- ENKU Base full kit: about **194 PLN/unit**
- ENKU Cover kit: about **213 PLN/unit**
- ENKU Pro kit: about **242 PLN/unit**

These figures include an 8% contingency/yield reserve and currently assume the display, assembled mainboard, BMI270, Hall sensor, battery, enclosure, controls and simple packaging. Pro additionally includes a placeholder frontlight optical/electrical delta.

They are not purchase quotes and will be replaced with actual PCBWay/Good Display/enclosure supplier pricing.

## Campaign price placeholders

For Kickstarter planning only:

- Electronics Kit: **299 PLN**
- Base Kit: **349 PLN**
- Cover Kit: **399 PLN**
- Pro Kit: **449 PLN**

The spreadsheet model reserves Kickstarter/platform processing fees and a configurable VAT buffer. Outbound shipping, campaign advertising, certification, returns and tooling are excluded and must be modeled separately before launch.

## Cost-down opportunities

Priority order:

1. direct panel/project pricing
2. PCB assembly quote at 50/100/250 units
3. replace premium microSD/FPC connectors with qualified lower-cost equivalents
4. source ESP32-S3 module through high-volume distributor/assembler channel
5. volume-price BMI270
6. standardize battery and cable
7. injection-molded or batch-printed enclosure only after demand is proven
8. optimize frontlight optics separately for Pro

## Variant philosophy

### Base
The campaign anchor: lowest-cost complete reader while retaining BMI270 and physical reading controls.

### Cover
Adds folio/magnets/Hall-driven wake-sleep with no core PCB redesign.

### Pro
Adds the frontlight optical stack and populated frontlight electronics while keeping the same compute platform.

## Manufacturing gates before Kickstarter

Do not publish a fixed reward price until:

- Good Display panel quote is received
- PCBWay or equivalent turnkey quote is received
- R0.1 is electrically validated
- enclosure print time/material cost is measured
- battery source is locked
- packaging is tested
- certification/compliance path is understood
- fulfillment shipping prices are known
- at least 10–15% total production risk reserve remains after all fees/tax assumptions

The working spreadsheet is `ENKU_Mainboard_R0.1_Cost_and_Kickstarter_Model.xlsx`.
