# ENKU R0.1 — PCBWay prototype manufacturing plan

> **Frontlight provenance gate (2026-10-05):** any October 2 TPS923610/10 µH/15 Ω/selector baseline in this document is historical, not fabrication-authoritative. Keep the Pro frontlight population DNP until the clean manufacturer-source re-derivation in `docs/HARDWARE_PROVENANCE.md` is complete.

Status: **prototype manufacturing baseline / sponsorship-order path**

## Scope

ENKU R0.1 prototype fabrication and assembly are planned around **PCBWay** as the preferred manufacturing path.

This is a prototype-development assumption, not a production-margin assumption.

PCBWay's current sponsorship program explicitly supports PCB-based engineering/open-source projects and can cover PCB fabrication and assembly through the sponsorship/coupon workflow.

Official references:
- https://www.pcbway.com/project/sponsor/learnsponsor.aspx
- https://www.pcbway.com/project/

## Economic rule

Two cost models must be maintained separately.

### A. Prototype cash cost

What ENKU actually pays for R0.1 / R0.2 prototypes after any sponsorship credit.

This may be close to zero for some fabrication/assembly items.

Use it for:
- prototype budget;
- number of revisions we can afford;
- experimental variants;
- destructive validation units.

### B. Normalized production COGS

What the same board would cost **without sponsorship** at:
- 10 units;
- 25 units;
- 50 units;
- 100 units.

Use it for:
- website kit pricing;
- Kickstarter pricing;
- margin decisions;
- component cost-down;
- whether a feature belongs in Base / Pro / DNP.

**Never use sponsored prototype cost to justify retail pricing.**

## First-order strategy

Sponsorship changes how aggressively ENKU can prototype, not what production quality should be.

Use the opportunity to:
- assemble the difficult small-pitch parts professionally;
- test multiple PCB revisions instead of over-freezing R0.1;
- populate instrumentation/test points;
- build both Base and Pro test populations;
- build at least one intentionally instrumented power/EPD validation unit.

Do not use sponsorship to:
- add unnecessary expensive components;
- ignore production BOM ceilings;
- create a PCB that only makes sense when free;
- overcomplicate the first revision.

## PCB target

R0.1 target:
- 4-layer PCB;
- ENIG preferred for exposed Dock contact pads and prototype reliability;
- lead-free assembly preferred if the incremental prototype cost is acceptable;
- mostly SMT assembly;
- board-side Dock pads exposed and ENIG-finished;
- test points accessible after assembly;
- antenna keepout obeyed;
- no copper/ground where Qi coil / RF / EPD reference design requires keepout.

Final board thickness is mechanical-design dependent and must be selected with enclosure/display stack.

## Assembly strategy

PCBWay assembly should populate the dense / precision components:

- ESP32-S3-WROOM-1-N16R8;
- BQ25185;
- TPS2121;
- TPS63802DLAR;
- BMI270;
- Hall sensor;
- USB ESD;
- USB-C;
- microSD connector if assembly-compatible;
- EPD FPC connector;
- EPD-HV MOSFET / diodes / passives;
- frontlight driver components on Pro prototype;
- Qi receiver / matching components on a dedicated Pro Wireless validation population.

Items likely installed after PCB assembly:
- raw e-paper panel;
- frontlight panel FPC;
- Li-Po battery;
- enclosure buttons / rocker caps;
- Dock spring contacts (Dock PCB/mechanics);
- Qi coil/ferrite if mechanically separate.

## Prototype population plan

Preferred first order if sponsorship / budget allows:

### 3 × Base electrical R0.1
Purpose:
- bring-up;
- power measurement;
- EPD rail validation;
- SD/Wi-Fi stress;
- firmware development.

### 2 × Pro electrical R0.1
Purpose:
- GDEY0426T82-FL01C compatibility;
- warm/cool frontlight validation;
- frontlight thermal / minimum brightness testing.

### 1 × Pro Wireless experimental
Purpose:
- Qi receiver;
- coil/ferrite geometry;
- wired/Qi input interaction;
- thermal test.

### Optional 1 × instrumentation board
Purpose:
- extra test headers;
- removable 0-ohm current links;
- rail measurement;
- easy power-path isolation.

If assembly constraints or sponsorship value make variants expensive, order one common PCB and vary population through DNP.

## DFM rules before upload

No PCBWay order until:

- schematic ERC passes;
- PCB DRC passes;
- all footprints verified against manufacturer drawings;
- FPC contact side is confirmed;
- USB-C mechanical outline checked;
- microSD insertion direction checked;
- ESP32 antenna keepout exists on all copper layers as required;
- EPD-HV creepage/spacing reviewed;
- exposed Dock pads are identified in fab notes;
- polarity/orientation marks exist for battery, diodes, IC pin 1 and FPC;
- no production-critical component is represented by a generic footprint without dimensional verification.

## Manufacturing package

The repository / order package should contain:

1. KiCad source:
   - .kicad_sch
   - .kicad_pcb
   - project settings

2. Gerbers:
   - copper layers;
   - solder mask;
   - silkscreen;
   - Edge.Cuts;
   - paste where required.

3. Drill files.

4. BOM:
   - designators;
   - MPN;
   - manufacturer;
   - quantity;
   - DNP state;
   - variant population;
   - approved alternates where safe.

5. CPL / pick-and-place:
   - reference;
   - X/Y;
   - rotation;
   - layer.

6. Assembly drawing.

7. Fabrication notes:
   - layer stack;
   - thickness;
   - copper;
   - finish;
   - exposed ENIG Dock pads;
   - impedance requirements if any;
   - controlled no-copper/keepout areas.

8. Schematic PDF.

9. Variant README:
   - Base;
   - Pro;
   - Pro Wireless;
   - exactly which designators are DNP for each.

## Component sourcing policy

For prototype assembly:

1. Prefer parts PCBWay can source directly when the exact MPN is available and cost is reasonable.
2. Allow customer-supplied / consigned parts for:
   - unusual FPC connectors;
   - difficult Good Display support components;
   - parts with poor assembler-channel availability.
3. Do not silently substitute:
   - charger / power-path IC;
   - EPD-HV switching parts;
   - regulator;
   - USB ESD;
   - frontlight current-setting components;
   - Qi matching network.

Any substitute in those blocks requires engineering approval.

## Design-for-test requirements

R0.1 should expose test points for at least:

- GND;
- VBUS_USB;
- VBUS_DOCK;
- VIN_CHARGER;
- VBAT;
- VSYS;
- 3V3_SYS;
- EPD VGH;
- EPD VGL;
- EPD VSH1;
- EPD VSH2;
- EPD VSL;
- VCOM where safe/appropriate;
- EPD BUSY;
- EPD RST;
- SPI SCLK;
- SPI MOSI;
- SD CS;
- EPD CS;
- DOCK_DETECT;
- frontlight warm output / sense;
- frontlight cool output / sense;
- Qi 5V output on Pro Wireless.

HV test points must be designed so accidental probe shorts are unlikely.

## Current-measurement features

Add removable / measurable current breakpoints where board area permits:

- 0 Ω link between BQ25185 SYS and regulator input;
- optional 0 Ω link for EPD-HV input;
- optional frontlight current measurement link.

This makes prototype power characterization much easier and costs very little.

Production may keep or remove these links after validation.

## PCBWay sponsorship publication strategy

If PCBWay sponsorship is approved:

- credit PCBWay transparently as prototype fabrication / assembly sponsor;
- publish photographs of the real fabricated board;
- publish the engineering results, including failures and revisions;
- link the open hardware files once R0.1/R0.2 is validated enough not to mislead builders.

Do not describe PCBWay as a confirmed sponsor until approval exists.

## Kickstarter implication

A sponsored R0.1 helps engineering but cannot be the basis of campaign margin.

Before Kickstarter:
- obtain a normal unsponsored quote at 25 / 50 / 100 units;
- include assembly and sourcing;
- include expected yield;
- keep display, battery, enclosure, packaging and fulfillment separate;
- re-run Base / Cover / Pro / Pro Wireless margins.

## R0.1 release gate

R0.1 can be sent to PCBWay only when:

- current Good Display pinouts are directly verified;
- EPD-HV exact parts are selected;
- BQ25185 external components are frozen;
- TPS63802 peak-current margin has a defensible calculation / prototype plan;
- all variant DNP choices are explicit;
- schematic review checklist is complete;
- fabrication outputs are generated from the actual KiCad board, not hand-made preview Gerbers.
