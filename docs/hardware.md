# ENKU Hardware Baseline

This document tracks the current hardware target and separates **verified facts** from **planned or unverified choices**.

## Reference platform

| Area | Current target | Status |
| --- | --- | --- |
| Main board | Waveshare ESP32-S3-ePaper-3.97 | Selected / ordered |
| MCU | ESP32-S3 | Platform-defined |
| Display | 3.97″ e-paper, 800 × 480 | Platform-defined |
| Input | Physical controls, non-touch UI | Product direction fixed; mapping to verify |
| Storage | microSD / local file workflow | To verify on real board |
| Connectivity | Wi-Fi / BLE capability | Platform-defined; ENKU Wi-Fi behavior planned |
| Battery | ~2000 mAh Li-Po target | Final pack TBD |
| Charging | Board-integrated behavior | To verify |
| Enclosure | Custom 3D-printable case | Starts after measurement |

## What will be verified on arrival

### Board and controls

- exact production revision;
- onboard control type and mapping;
- GPIO behavior where relevant;
- wake-capable inputs;
- connector placement.

### Display

- actual usable refresh modes;
- full-refresh time;
- partial-refresh support and limits;
- ghosting behavior;
- contrast in real reading layouts;
- effect of repeated UI updates.

### Storage

- microSD wiring and interface;
- filesystem support;
- file size / performance limits;
- safe-write behavior during power loss.

### Power

- charging behavior;
- active current;
- reading/idle current;
- sleep current;
- wake behavior;
- practical battery runtime;
- battery voltage/percentage mapping.

## Battery target

The working target is a **single-cell 3.7 V Li-Po around 2000 mAh**, thin enough for a compact reader enclosure.

A previously considered mechanical envelope is approximately **5 × 50 × 60 mm**, but this is not yet a final specification.

Final selection depends on:

- real internal space after board measurement;
- connector type and polarity;
- board charging limits;
- protection circuit;
- thermal/mechanical clearance;
- enclosure thickness target.

No battery model should be treated as final until physically validated.

## Mechanical verification

Before starting the final enclosure, record:

- PCB outline;
- display active area and glass outline;
- total display stack thickness;
- board/display offsets;
- connector protrusions;
- control travel and access;
- screw/mounting features;
- battery keep-out area;
- cable bend radii;
- safe bezel overlap.

The enclosure will be based on these measurements rather than catalog assumptions.

## Planned enclosure variants

The first goal is **one reliable baseline case**.

Only after that is stable, possible variants may include:

- different battery thickness/capacity;
- alternative back shell;
- stand/cover attachment;
- revised button geometry.

The project will avoid fragmenting into many mechanical variants before the baseline design is proven.

## Hardware contribution opportunities

Contributors with experience in the following areas are especially useful:

- ESP32-S3 low-power work;
- Waveshare e-paper drivers;
- e-paper ghosting/partial refresh characterization;
- battery/power measurement;
- compact Li-Po integration;
- mechanical CAD and 3D-print tolerances;
- button and enclosure ergonomics.

See [CONTRIBUTING.md](../CONTRIBUTING.md).
