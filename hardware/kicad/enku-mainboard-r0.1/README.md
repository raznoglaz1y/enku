# ENKU Mainboard R0.1 — KiCad worktree

This directory contains the CAD worktree for the custom ENKU mainboard.

Current state:
- portrait 54 × 94 mm four-layer PCB;
- mounting-hole baseline;
- ESP32 antenna, Qi, buttons, microSD, USB-C, Dock and hard-power zones;
- first mechanically sensitive component placement has started;
- J_EPD uses the R0.1 dual-contact 24-pin connector baseline;
- J_FL uses the R0.1 dual-contact 6-pin connector baseline;
- EPD 47 µH inductor and boost MOSFET are now represented in the placement;
- Pro frontlight driver zone and remap field are represented;
- no routing yet.

Important:
- schematic capture remains authoritative for nets and final footprint synchronization;
- several footprints in the PCB are marked as R0.1 placement placeholders and must be replaced/verified against manufacturer land patterns before fabrication;
- no KiCad ERC/DRC has been run in the current environment;
- this is **not fabrication-ready**.

Companion:
- [Placement baseline A](PLACEMENT_R01.md)
- `../../../docs/schematic-netlist-r01.md`
- `../../../docs/r01-blocker-closure.md`

Next:
1. capture the complete schematic;
2. assign/verify exact manufacturer footprints;
3. update the PCB from schematic;
4. reconcile the first placement with actual courtyard/keepout data;
5. route;
6. run ERC/DRC in KiCad;
7. generate PCBWay files only from the checked KiCad project.


## Schematic hierarchy

R0.1 now has a real KiCad hierarchical capture baseline:

- `enku-mainboard-r0.1.kicad_sch` — root sheet
- `power.kicad_sch` — USB/Dock arbitration, charger, hard-off 3.3 V path
- `mcu_io.kicad_sch` — ESP32-S3-WROOM-1-N16R8, shared SPI, I2C, buttons, Hall/IMU/ADC assignments
- `epd_hv.kicad_sch` — 24-pin EPD FPC and SSD1677 external-HV interface boundary
- `frontlight.kicad_sch` — TPS923610 path, 6-pin raw FPC and 0R/DNP remap boundary
- `connectors.kicad_sch` — USB-C, dock, debug and DNP Qi interface

These files are schematic-capture baselines, not a fabrication release. The next gate is to expand the remaining block-level circuits into exact discrete symbols, review every footprint, and run KiCad ERC before routing is treated as electrically authoritative.
