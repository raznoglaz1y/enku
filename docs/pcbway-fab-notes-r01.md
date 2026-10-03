# ENKU R0.1 — PCBWay fabrication notes

Status: **working fabrication note set; required before first-spin order**

These notes are constraints for CAM/assembly review. They do not replace the Gerbers, drill files, BOM, CPL or assembly drawings.

## USB-C — GCT USB4105-GF-A-120

J_USB uses the manufacturer-recommended USB4105 geometry:
- 2 × Ø0.65 mm locating NPTH;
- duplicated outer GND lands A1/B12 and A12/B1: 0.60 × 1.15 mm;
- locating-hole and signal-land coordinates are kept unchanged from the GCT recommended PCB layout / KiCad manufacturer-specific footprint.

KiCad resolves the closest locating-NPTH to outer-GND-pad copper spacing as **0.1944 mm**.

PCBWay's published manufacturing table gives:
- normal NPTH-to-copper target: **0.20 mm**;
- minimum NPTH-to-copper capability: **0.15 mm**.

For R0.1 this is an intentional, connector-local exception. The project custom rule permits **0.19 mm only for item pairs that both belong to J_USB**.

### CAM instruction

**Do not automatically trim or move the A1/B12 or A12/B1 GND lands to create 0.20 mm clearance.**

Instead:
1. retain the submitted GCT USB4105 land geometry;
2. confirm that 0.1944 mm NPTH-to-copper is accepted for the selected 4-layer process;
3. contact us before changing connector copper or locating-hole geometry.

Changing those pads without approval can reduce connector solder fillet area or make the fabricated footprint differ from the manufacturer-recommended layout.

## Assembly orientation review

Before PCBA release, visually verify pin-1/orientation against the assembly drawing for:
- U_SRC — TPS2121;
- U_CHG — BQ25185;
- U_3V3 — TPS63802;
- U_FL — TPS923610;
- U_IMU — BMI270;
- J_EPD — 24-pin Hirose;
- J_FL — 6-pin Hirose;
- J_SD — Hirose DM3AT-SF-PEJM5;
- J_USB — GCT USB4105.

The CPL preview is not accepted as the sole source of truth for these orientation-critical parts.

## Do not manufacture from an intermediate commit

Release only from the dedicated fabrication tag after:
- native ERC is reviewed/clean;
- routing is complete;
- DRC fabrication gate passes;
- no unrouted nets remain;
- exact enclosure-dependent battery/button/power-switch footprints are frozen;
- Gerbers/drills have had an independent visual review.
