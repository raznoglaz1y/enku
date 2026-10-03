# ENKU Mainboard R0.1 — KiCad worktree

This directory contains the CAD worktree for the custom ENKU mainboard.

Current state:
- portrait 54 × 94 mm four-layer PCB shell;
- mounting-hole baseline;
- marked regions for ESP32 antenna, EPD power/FPC, buttons, microSD, USB-C, Dock and Qi reserve;
- schematic capture will become the authoritative source for footprints and nets.

The PCB file is a placement shell, not a release board.

Next:
1. capture the schematic from docs/schematic-netlist-r01.md;
2. assign exact manufacturer footprints;
3. update PCB from schematic;
4. place and route;
5. run ERC/DRC;
6. export production files only from the checked KiCad project.
