# ENKU Project Status

Last updated: October 2026.

## Current stage

**Firmware integration / custom R0.1 routing / pre-production verification**

ENKU is not yet a hardware release, but the project is well beyond product definition. The reader runtime and ESP-IDF platform are implemented in parallel with a dedicated R0.1 mainboard. The PCB is now in late routing and verification: the power cluster and USB-C connector-side CC/ESD work have clean KiCad checkpoints, while USB data closure, dock/unrouted closure and the final manufacturing audit remain.

## Completed

- project name and visual identity;
- reference Waveshare ESP32-S3-ePaper-3.97 platform selected;
- public GitHub repository;
- initial 58-screen UI baseline audited;
- EN set as canonical/source UI language;
- localization scope defined;
- reading presets defined;
- non-touch focus/navigation model defined;
- reusable Figma foundations;
- Statusbar component;
- Header component;
- Primary Button component;
- core UI rules documented;
- Library/Reading/Settings/Storage/Wi-Fi behavior specified;
- local browser-management scope defined;
- firmware architecture direction documented;
- enclosure development workflow defined.

## In progress

- shared Figma component system;
- Universal Row component;
- canonical EN screen redesign;
- Storage/Import screen revision;
- Wi-Fi screen revision;
- hardware sourcing and delivery;
- supplier and hardware-partner outreach.

## Current hardware gate

- keep every accepted PCB checkpoint green in native KiCad CI;
- complete USB D+/D− from Type-C through ESD/series tuning to ESP32-S3;
- close dock and every remaining unrouted connection;
- require an explicit zero-unrouted result in addition to DRC/ERC;
- audit power/EPD-HV return paths, antenna keepout, connector access, footprints and repairability;
- generate and inspect Gerber, drill, BOM and pick-and-place outputs before PCBWay prototype submission.

## Next

1. close USB and dock routing;
2. reach zero unrouted connections with all hardware CI green;
3. run the full electrical/mechanical/manufacturability audit;
4. generate the R0.1 manufacturing package and PCBWay preview;
5. assemble the first prototypes and perform measured hardware bring-up;
6. feed real display, storage, power and mechanical measurements back into firmware and enclosure work.

## Looking for hardware partners and suppliers

ENKU is actively looking for companies and distributors interested in supporting the project with hardware and components.

Current priorities include:

- Waveshare ESP32-S3-ePaper-3.97 boards for development and testing;
- suitable Li-Po batteries for the final enclosure;
- switches, buttons, fasteners and small mechanical/electronic components;
- prototype parts and replacement units for testing;
- sample units or project pricing;
- technical specifications and integration support;
- suppliers willing to support future hardware and enclosure iterations.

Where appropriate, participating companies can be listed in the repository and project documentation as hardware partners or component suppliers.

The project prefers components that are easy for future builders to source again, so availability and reproducibility are important selection criteria.
