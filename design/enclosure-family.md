# ENKU enclosure family

Status: concept specification for future CAD work.

The enclosure family is intentionally based on one main PCB wherever possible.

## Common physical language

All variants share:
- portrait-first layout
- bottom-center USB-C
- top-left hardware power switch
- right-side page controls
- 3.97-inch display
- compact rounded enclosure
- replaceable rear shell
- printable prototype geometry

## Variant A — Slim

Reference open-source shell.

Target character:
- minimal
- light
- inexpensive
- easy to print
- easy to disassemble

Suggested construction:
- front bezel/frame
- PCB/display carrier
- rear shell
- four M2 fasteners or snap-fit + limited fasteners

No permanent cover or frontlight.

## Variant B — Grip

Ergonomic one-hand shell.

Changes from Slim:
- wider right-hand grip zone
- gentle rear bulge or wedge around the palm
- larger page rocker cap
- slightly deeper local cavity around buttons
- battery positioned to avoid top-heavy balance

The device can be rotated 180 degrees for left-hand operation.

## Variant C — Cover

Slim/Grip-compatible magnetic folio.

Elements:
- replaceable magnetic front flap
- Hall wake/sleep magnet
- magnetic closure point
- protective lip around display
- relief around hardware power switch
- button guard geometry to reduce presses in a bag

The magnet must be tested for interference and its final position must follow Hall-sensor placement.

## Variant D — Pro

Illuminated version.

Changes:
- frontlight light guide / diffuser stack
- LEDs and light-management geometry
- potentially slightly thicker front bezel
- populated frontlight electronics
- Hall sensor and cover support standard

A Pro Cover may share the same external attachment strategy as Cover if thickness permits.

## CAD deliverables planned

For each variant:
- STEP
- STL
- exploded assembly render
- dimension drawing
- printable prototype profile
- button-cap/rocker part
- battery pocket definition
- PCB mounting points

Cover-specific:
- folio front
- hinge
- magnet pockets
- Hall alignment drawing

Pro-specific:
- frontlight stack cross-section
- LED/light-guide carrier
- thermal/optical clearance model

## Design rule

Case concepts must follow the PCB and display stack. Do not distort the board solely to make a render look cleaner; mechanical and electrical constraints stay authoritative.
