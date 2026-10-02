# ENKU Project Status

Last updated: October 2026.

## Current stage

**Product definition / UI system / hardware preparation**

ENKU is not yet a working reader firmware release. The project currently has a defined product direction, UI specification, interaction model, reference hardware and a detailed implementation roadmap.

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
- contributor and hardware-partner outreach.

## Next

1. receive and inspect the target hardware;
2. verify controls, display, storage and power behavior;
3. start minimal firmware bring-up;
4. profile refresh/memory/power;
5. lock the first reader format and parser approach;
6. implement the shared UI/focus layer;
7. build the offline Library + Reader MVP;
8. start the first measured enclosure prototype.

## Looking for contributors

ENKU is actively looking for contributors.

The project is especially interested in people with experience in:

- ESP32-S3 firmware;
- e-paper display drivers;
- embedded UI architecture;
- low-power embedded systems;
- EPUB/FB2/text parsing and pagination;
- font rendering and Unicode;
- local web interfaces on embedded devices;
- localization;
- CAD / 3D-printable enclosure design;
- technical documentation.

You do not need to commit to the whole project. A focused contribution, measurement, review or proof-of-concept is useful.

Start with [CONTRIBUTING.md](../CONTRIBUTING.md) or open an issue describing the area you would like to help with.
