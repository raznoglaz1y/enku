# ENKU BOM / Parts Status

This document tracks the current component plan for ENKU and makes a clear distinction between selected hardware, parts awaiting validation, and components that still need sourcing.

The BOM will remain intentionally conservative until the real hardware has been measured and tested.

## Current parts

| Category | Part / target | Status | Notes |
| --- | --- | --- | --- |
| Main board | Waveshare ESP32-S3-ePaper-3.97 | **Selected / ordered** | Reference hardware platform |
| MCU | ESP32-S3 | **Fixed by platform** | Firmware target |
| Display | 3.97″ e-paper, 800 × 480 | **Fixed by platform** | Portrait + landscape UI |
| Storage | microSD / local storage | **To verify** | Platform capability; firmware/filesystem behavior to test |
| Connectivity | Wi-Fi | **Planned** | Optional for reading; used for local management |
| Connectivity | BLE | **Available on platform** | No committed ENKU feature yet |
| Battery | Single-cell 3.7 V Li-Po, around 2000 mAh | **TBD** | Final dimensions, connector and runtime pending |
| Power switch | Compact hard power switch | **TBD** | Mechanical and electrical implementation pending |
| Controls | Physical buttons / onboard controls | **To verify** | Exact mapping and wake behavior pending |
| Fasteners | M2-class or similar | **TBD** | Final choice after mechanical measurement |
| Enclosure | 3D-printable custom case | **Planned** | Starts after board/display measurement |

## Battery sourcing target

The current working target is:

- single-cell Li-Po;
- 3.7 V nominal;
- around 2000 mAh;
- protected pack preferred;
- thin rectangular format suitable for a compact enclosure;
- connector compatible with the final board/power architecture.

A mechanical envelope around **5 × 50 × 60 mm** has been considered, but this is **not final**.

Before locking the battery, ENKU will verify:

- actual free volume inside the enclosure;
- connector type;
- connector polarity;
- board charge-current limits;
- protection circuit;
- cable routing;
- thermal clearance;
- effect on final device thickness.

## Supplier priorities

ENKU is currently interested in suppliers that can provide one or more of the following:

### Reference hardware

- Waveshare ESP32-S3-ePaper-3.97 development boards;
- replacement units for testing;
- access to accurate technical documentation;
- project/sample pricing.

### Power

- suitable 3.7 V Li-Po packs;
- documented connector/polarity;
- dimensional drawings;
- protection-circuit information;
- repeatable availability.

### Mechanical components

- compact power switches;
- tactile buttons where needed;
- small screws and fasteners;
- enclosure hardware;
- prototype quantities without large MOQ requirements.

## Sourcing principles

Parts selected for ENKU should ideally be:

- realistically available in Europe;
- purchasable in small quantities;
- documented;
- replaceable by future builders;
- not tied to one temporary marketplace listing;
- consistent enough for repeatable enclosure and firmware work.

The project will prefer reproducibility over selecting a rare component solely because it is slightly smaller or cheaper.

## Partner recognition

Where appropriate, companies supporting ENKU with evaluation hardware, sample components, project pricing or technical support can be listed in project documentation as:

- **Hardware Partner**
- **Component Supplier**
- **Development Hardware Supplier**

This recognition does not imply technical endorsement of untested parts. Components are still validated before they become part of the reference BOM.

## What is intentionally not fixed yet

The following should remain open until hardware testing:

- exact battery model;
- exact battery connector;
- final power switch;
- final fastener dimensions;
- final enclosure hardware;
- final case dimensions;
- alternate board revisions.

See [Hardware baseline](hardware.md) for the validation plan.
