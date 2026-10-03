# ENKU multi-variant BOM architecture

Status: **pre-schematic cost architecture**

This document defines what should be shared across ENKU variants before individual manufacturer part numbers are frozen.

## Design objective

Use one core mainboard wherever practical.

Different products should be created mainly by:
- display choice;
- component population / DNP;
- enclosure;
- cover;
- wireless coil;
- frontlight optical stack.

Avoid maintaining independent Base and Pro compute boards unless panel integration proves it unavoidable.

## Shared core — all reader variants

Target blocks:

- ESP32-S3-WROOM-1-N16R8;
- native USB-C;
- protected charge / system-power path;
- 1S Li-Po connector;
- hard hardware power-off control;
- microSD connector accessible from enclosure edge;
- e-paper logic interface;
- EPD HV/analog power section based on validated Good Display reference;
- Previous / Next rocker electrical switches;
- Select / Menu;
- Back;
- BMI270;
- low-power Hall sensor footprint;
- battery measurement;
- Dock pogo pads;
- protected dock-input OR / power mux;
- DOCK_DETECT / accessory ID;
- debug / factory pads;
- frontlight control / connector footprint reserved where routing allows;
- Qi interface / keepout reserved where it does not compromise RF, EPD or mechanical design.

## Variant population matrix

| Block | Electronics | Base | Cover | Pro | Pro Wireless |
| --- | --- | --- | --- | --- | --- |
| ESP32-S3 core | yes | yes | yes | yes | yes |
| microSD | yes | yes | yes | yes | yes |
| BMI270 | yes | yes | yes | yes | yes |
| USB-C | yes | yes | yes | yes | yes |
| Dock pogo | yes | yes | yes | yes | yes |
| Dock detect / mux | yes | yes | yes | yes | yes |
| Hall sensor | optional | optional / preferred | yes | yes | yes |
| Frontlight driver | DNP | DNP | DNP | yes | yes |
| Frontlight connector | reserve | reserve | reserve | yes | yes |
| Qi receiver / matching | DNP | DNP | DNP | DNP | yes |
| Qi coil | no | no | no | no | yes |
| Magnetic cover | no | no | yes | optional | optional / yes |
| Display | Base | Base | Base | Pro FL | Pro FL |

## Display mapping

### Base family

GDEY0397T81P

Used by:
- Electronics Kit;
- Base;
- Cover.

### Pro family

GDEY0426T82-FL01C

Used by:
- Pro;
- Pro Wireless.

The custom board should aim to support both SSD1677-based panels through the same logical display interface, but final FPC pinout and HV requirements must be verified before this is treated as electrically interchangeable.

## Cost envelopes

Landed BOM ceilings:

- Electronics Kit: PLN 155
- Base: PLN 180
- Cover: PLN 200
- Pro: PLN 230
- Pro Wireless: PLN 260
- Dock: PLN 40

These include the intent to leave room for:
- assembly losses / rework;
- packaging-level small parts;
- small-batch sourcing inefficiency.

They are not final accounting COGS.

## Cost priority by block

### Protect aggressively

These blocks must be cost-optimized early:
- display;
- ESP32 module;
- PCB + assembly;
- battery;
- microSD connector;
- EPD HV section;
- enclosure.

### Spend where UX justifies it

Do not cost-cut below acceptable user experience on:
- page switches / rocker tactile quality;
- USB-C mechanical robustness;
- power-path safety;
- battery protection;
- frontlight optical quality;
- Dock contact reliability.

### Keep optional

Cost-heavy or mechanically risky features:
- frontlight;
- Qi;
- premium cover;
- optional encoder footprint;
- higher-end battery gauge.

## BOM decision gates before final routing

### Gate 1 — Displays

Need:
- Good Display pinout/reference circuit;
- quantity pricing 10 / 25 / 50 / 100;
- long-term availability;
- mechanical drawing;
- frontlight electrical requirements for Pro.

### Gate 2 — Power

Resolve:
- system regulator current margin;
- charger versus charger + true power-path;
- USB and Dock simultaneous-input behavior;
- hard-OFF charging;
- Qi interaction with wired charger.

### Gate 3 — EPD HV

No fabrication until:
- official reference circuit is integrated;
- component ratings and land patterns are verified.

### Gate 4 — Input mechanics

Choose actual tact switches after:
- actuation force comparison;
- noise comparison;
- enclosure rocker geometry;
- cycle life and sourcing review.

### Gate 5 — microSD

Choose connector only after:
- side-entry geometry is frozen;
- push-push versus push-pull cost is compared;
- card accessibility is verified with enclosure wall thickness.

### Gate 6 — Dock

Choose:
- pogo pitch/contact geometry;
- alignment strategy;
- input OR/mux IC;
- accessory ID scheme.

### Gate 7 — Pro frontlight

Verify:
- frontlight driver topology;
- warm/cool channel current;
- PWM behavior;
- minimum usable night brightness;
- thermal rise.

### Gate 8 — Qi

Only for Pro Wireless:
- receiver IC / module decision;
- coil size;
- ferrite;
- keepout;
- thermal test;
- Qi interoperability/certification strategy.

## BOM freeze rule

The schematic may progress before exact supplier pricing is final.

The PCB **must not** be declared production-ready until:
- each MUST component has an exact MPN;
- each variant has a complete populated/DNP BOM;
- supplier or assembly pricing demonstrates that the cost ceilings are realistic;
- no BLOCKER item remains in the EPD/power path.


## R0.1 power candidate

The first power-path candidate is now **TI BQ25185**.

Reason:
- real power-path management is now required by USB-C + Dock + hard-OFF charging;
- low battery-only quiescent current;
- up to 1 A charging;
- system-load support while charging;
- wider input tolerance than the earlier simple charger placeholder.

The earlier MCP73831 should no longer be treated as the preferred production architecture.

USB-C and Dock inputs will be combined through protected OR-ing / source selection before the charger. Pro Wireless should preferably use a Qi receiver that supplies the same common charger path rather than introducing an independent second battery charger.

Detailed rationale:
[Custom mainboard power R0.1](custom-mainboard-power-r01.md)


## Power quality / cost shortlist

Current R0.1 shortlist:

| Block | Candidate | Status |
| --- | --- | --- |
| Charger + power path | TI BQ25185 | Primary |
| USB / Dock input mux | TI TPS2121 | Quality baseline; compare against qualified discrete OR-ing |
| 3.3 V regulator | TI TPS63031 | Primary bench-test candidate |
| 3.3 V high-current fallback | TI TPS63070 | Use only if TPS63031 burst margin is inadequate |
| Hall cover sensor | TI DRV5032 family | Keep where feature is populated |
| Qi receiver | TI BQ51013C | Pro Wireless only |

Cost policy:
- do not remove a safety/reliability part for a trivial PLN saving;
- prefer integrated solutions where they reduce failure modes, test burden or support cost;
- cost-down first through sourcing, DNP population, common PCB, qualified connector substitutions and volume purchasing;
- Base BOM must not carry Pro/Qi cost.

See:
- [Power architecture R0.1](custom-mainboard-power-r01.md)
- [Power cost pass R0.1](power-cost-pass-r01.md)
- [Open-source commercialization](open-source-commercialization.md)


## Schematic-level power baseline

Current preferred R0.1 implementation:

- BQ25185 charger / PowerPath;
- initial 4.2 V / 500 mA input-limit configuration;
- initial 300 mA fast-charge target for prototype thermal validation;
- TPS2121 as the USB/Dock quality-baseline source mux;
- TPS63031 fixed 3.3 V buck-boost as the first system-regulator candidate;
- physical hard OFF drives the switched system rail off while leaving charging available;
- BQ51013C-family Qi receiver only for Pro Wireless;
- Pro frontlight prototype reserves two independent boost LED channels, with TPS61165 as the quality-baseline reference candidate.

Detailed implementation:
- [Power schematic R0.1](power-schematic-r01.md)
- [Pro frontlight R0.1](frontlight-r01.md)
- [Power cost pass](power-cost-pass-r01.md)

EPD-HV remains a BLOCKER until the official panel reference circuit is verified.


## EPD / pin-map baseline

New schematic inputs:
- [EPD integration R0.1](epd-integration-r01.md)
- [ESP32-S3 pin map R0.1](pin-map-r01.md)

Current direction:
- one shared SPI bus for EPD + microSD with separate chip selects;
- current 24-pin SSD1677 family pinout used as the schematic baseline;
- current Good Display panel PDFs still must be checked directly before connector orientation / HV values are frozen;
- GPIO35–37 are reserved for the N16R8 memory topology;
- native USB remains on GPIO19/20;
- four direct page/navigation buttons remain separate GPIOs.


## Sponsored prototype vs production COGS

PCBWay is the preferred R0.1 fabrication / assembly path.

If prototype sponsorship is approved, maintain two independent cost columns:

1. **prototype cash cost** — actual project spend after sponsorship credit;
2. **normalized unsponsored COGS** — what the same hardware would cost without sponsorship.

Only normalized unsponsored COGS may be used for:
- Kickstarter margin;
- website kit pricing;
- BOM ceiling decisions;
- Base / Pro feature allocation.

Sponsored fabrication is an engineering accelerator, not a permanent unit-cost assumption.

Manufacturing plan:
[PCBWay prototype plan](pcbway-prototype-plan.md)


## Schematic capture baseline

The R0.1 schematic should now be captured from the named-net/block specification rather than directly from the early layout WIP:

[Schematic net/block baseline R0.1](schematic-netlist-r01.md)

Current display mechanical/electrical freeze state:
[Display freeze matrix R0.1](display-freeze-r01.md)


## EPD-HV sourcing baseline

The R0.1 SSD1677 reference circuit now has a concrete sourcing baseline:

- Q1: Vishay SI1308EDL-T1-GE3 preferred for prototype;
- D1-D3: MBR0530;
- R_RESE: 2.2 Ω / 1% / 0805;
- L1: 47 µH, exact production MPN still open;
- HV capacitors: 4.7 µF / 25 V / 0805 X5R/X7R and 1 µF / 25 V X7R according to the SSD1677 reference design.

Cost-down alternatives may only replace these after the first real EPD rail measurements.

See:
[EPD-HV components R0.1](epd-hv-components-r01.md)


## Pro frontlight production direction

The Pro frontlight architecture now prefers **one TPS923610 synchronous boost LED driver** with warm/cool return selection rather than two independent boost drivers.

Reason:
- substantially lower component/assembly count;
- active TI part;
- 24.5 V output headroom for the <=15 V Good Display strings;
- 200 mV current regulation;
- analog PWM dimming down to 0.1%;
- only a few PLN of IC cost.

The exact 6-pin frontlight mapping remains blocked on the current Good Display FL0426-S01C drawing.

See:
[Pro frontlight R0.1](frontlight-r01.md)


## Pro frontlight component baseline

R0.1 prototype parts now have a concrete baseline:
- TPS923610DRLR boost driver;
- 10 µH boost inductor, TDK VLS252012HBX-100M-1 as prototype candidate;
- 15 Ω / 1% current-set resistor (~13.3 mA nominal);
- BSS138-family low-side warm/cool selectors;
- 4.7 µF / 50 V output capacitor;
- 6-pin / 0.5 mm FPC connector class.

The exact frontlight connector pin map remains blocked on direct verification of the current Good Display FL0426-S01C drawing.

See:
[Frontlight components R0.1](frontlight-components-r01.md)


## R0.1 blocker-closure parts

The PCB-stage baseline now includes:
- EPD inductor: Laird TYS5040470M-10, 47 µH, 1 A rated / 1.1 A Isat;
- EPD boost switch: IRLML6346TRPBF preferred for prototype;
- EPD rectifiers: 40 V / 1 A Schottky class;
- EPD HV capacitors: 50 V class for R0.1 prototype margin;
- EPD connector: Hirose FH34SRJ-24S-0.5SH(50);
- Pro frontlight connector: Hirose FH34SRJ-6S-0.5SH(50) with a temporary remap matrix.

These choices prioritize first-board debugability and margin; production cost-down comes only after measured rail validation.
