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


## Vendor-confirmed controls

Current official Waveshare source identifies the onboard navigation inputs as active-low GPIOs with pull-ups:

| Control | GPIO | Current vendor naming |
| --- | ---: | --- |
| Up | GPIO4 | Button_Up |
| Function / press | GPIO5 | Button_Function |
| Down | GPIO6 | Button_Down |
| BOOT | GPIO0 | Boot |

The official ESP-IDF example polls these controls with a 5 ms timer and a software button state machine rather than GPIO interrupts.

The PWR key is handled through the AXP2101 PMU path. The current vendor example configures a 1 s power-on press and a 4 s hardware power-off press.

ENKU's logical mappings are documented in [Input & Physical Controls](input-model.md). These source-derived mappings will still be physically verified on the production board when it arrives.


## Vendor-confirmed power behavior

The current official Waveshare application distinguishes panel sleep from full PMU shutdown:

- `EPD_Sleep()` sends the e-paper controller into deep sleep, but the ESP32 application continues running;
- the current full vendor application does not use ESP32 deep sleep as its normal product sleep flow;
- full software power-off calls the AXP2101 shutdown path;
- the vendor PMU setup uses approximately 1 s PWR hold for power-on and 4 s for hardware power-off.

ENKU therefore treats **DisplayIdle**, **Suspended** and **PoweredOff** as separate product states.

The final suspend mechanism and wake sources remain pending real-board measurements. See [Power, Sleep & Wake Model](power-model.md).
