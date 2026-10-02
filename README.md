<p align="center">
  <img src="assets/branding/enku-logo.svg" alt="ENKU" width="420">
</p>

<h1 align="center">ENKU</h1>

<p align="center">
  <strong>Open-source compact e-reader built around a 3.97″ e-paper display and ESP32-S3.</strong>
</p>

<p align="center">
  A focused, low-power reading device with physical controls, a carefully designed interface, local file management and an open hardware/software roadmap.
</p>

---

## What is ENKU?

ENKU is an open-source e-reader project designed around the **Waveshare ESP32-S3-ePaper-3.97** platform with an **800 × 480** e-paper display.

The goal is not to reproduce a full tablet or build a feature-heavy general-purpose device. ENKU is intended to be a **small, calm and purpose-built reader**: fast enough for books, simple to operate, comfortable to use offline, and understandable from both the software and hardware side.

The project is being developed as a complete product system rather than only a firmware demo. It includes:

- firmware and device architecture;
- a complete non-touch UI for physical controls;
- library and reading workflows;
- typography and per-book settings;
- Wi-Fi and local browser-based file management;
- storage and import flows;
- sleep and power-management behavior;
- enclosure development and mechanical iterations;
- reproducible documentation for builders and contributors.

> **Current status:** design system, UX specification and project architecture are actively being developed. Firmware bring-up begins after the target hardware is in hand. Features listed below are planned unless explicitly marked as completed.

### Looking for hardware partners and component suppliers

ENKU is currently looking for **hardware partners, distributors and component suppliers** interested in supporting the development of the project.

The most useful forms of support at this stage are:

- Waveshare ESP32-S3-ePaper-3.97 development units;
- Li-Po batteries suitable for a compact e-reader enclosure;
- buttons, switches, fasteners and other enclosure hardware;
- prototype and replacement components for hardware testing;
- project discounts or sample units;
- technical information about supplied components;
- support for future hardware revisions and enclosure prototypes.

Partners can be credited in the project documentation and repository as hardware partners or component suppliers where appropriate.

The goal is to build ENKU around readily available, reproducible parts rather than one-off or inaccessible hardware.

For partnership or supply discussions, please open an issue or contact the project owner through GitHub.

## Project goals

ENKU is being designed around a deliberately narrow set of product goals:

- provide a **comfortable, distraction-free reading experience** on a compact e-paper device;
- remain useful **fully offline**;
- use **physical controls** instead of depending on a touchscreen;
- keep book storage, reading progress and settings under the user's control;
- support a clean local workflow for importing and managing books;
- make the firmware, hardware assumptions and enclosure work understandable and reproducible;
- stay small enough to be a practical everyday reader rather than a general-purpose tablet.

## Non-goals

ENKU is **not** intended to become:

- an Android-like general-purpose device;
- a multimedia tablet;
- a cloud-dependent reading service;
- a storefront or DRM ecosystem;
- a notification-heavy connected gadget;
- a platform that hides core reading features behind an account.

The project favors a smaller, coherent feature set over adding functionality that does not improve reading.

## Why ENKU?

### Reading first

The interface is built around one job: **reading**. The reader screen stays visually quiet, controls appear only when needed, and common actions are reachable without navigating through deep menus.

### E-paper from the beginning

ENKU is designed specifically for e-paper constraints rather than adapting a conventional LCD interface afterward. Refresh behavior, focus states, progress updates, sleep screens and interaction density are all treated as part of the product architecture.

### Physical controls, no touchscreen dependency

The UI is designed for buttons and deterministic focus navigation. Focus, selected values and active states are separate concepts, which keeps the interface predictable and accessible without touch.

### Offline by default

Books remain usable without Wi-Fi. Connectivity is an optional management layer, not a requirement for reading.

### Open and understandable

The project aims to document not only the final code, but also **why** design and implementation decisions were made. UI rules, navigation behavior, hardware assumptions, power decisions and enclosure revisions are tracked openly.

### Compact hardware target

A 3.97″ panel keeps the device small enough to carry easily while still providing a practical 800 × 480 reading canvas. Both portrait and landscape orientations are part of the design system.

## Design preview

The interface is being designed in English first and then localized. The boards below are current **EN design previews** from the active UI revision work; they are not yet firmware screenshots.

<table>
<tr>
<td width="50%"><img src="design/screens/en/ENKU_library_grid_v2.svg" alt="ENKU Library grid design preview"></td>
<td width="50%"><img src="design/screens/en/ENKU_first_start_language_v2.svg" alt="ENKU first-start language design preview"></td>
</tr>
<tr>
<td align="center"><strong>Library</strong></td>
<td align="center"><strong>First start / Language</strong></td>
</tr>
<tr>
<td width="50%"><img src="design/screens/en/ENKU_book_details_v2.svg" alt="ENKU Book details design preview"></td>
<td width="50%"><img src="design/screens/en/ENKU_library_list_v2.svg" alt="ENKU Library list design preview"></td>
</tr>
<tr>
<td align="center"><strong>Book details</strong></td>
<td align="center"><strong>Library / List</strong></td>
</tr>
</table>

The production visual source of truth remains the Figma design system and approved review boards.

---

## Target hardware

The current reference platform is:

| Component | Target |
| --- | --- |
| Main board | Waveshare ESP32-S3-ePaper-3.97 |
| MCU | ESP32-S3 |
| Display | 3.97″ e-paper, 800 × 480 |
| Input | Physical controls, non-touch UI |
| Storage | microSD / local storage workflow |
| Connectivity | Wi-Fi, BLE available at platform level |
| Power | Li-Po battery; final pack and runtime profile to be validated on hardware |
| Enclosure | Custom 3D-printable case, developed after mechanical verification |

Hardware-dependent details such as battery choice, wake behavior, button mapping, refresh modes and enclosure geometry remain provisional until verified on the real board.

### Parts status

| Part | Current status |
| --- | --- |
| Waveshare ESP32-S3-ePaper-3.97 | **Selected / ordered** |
| 3.97″ 800 × 480 e-paper panel | **Part of reference platform** |
| microSD storage | **Platform feature — hardware verification pending** |
| Li-Po battery | **TBD — ~2000 mAh target, final size/connector pending** |
| Physical controls | **Platform controls to be mapped and tested** |
| Power switch / enclosure hardware | **TBD** |
| Fasteners | **TBD after mechanical measurements** |
| 3D-printed enclosure | **Planned after hardware measurement** |

See the detailed [Hardware baseline](docs/hardware.md) and [BOM status](docs/bom.md).

## Planned reader experience

### Library

- grid and list views;
- persistent sort and filter state;
- title/author search;
- book details and cover handling;
- reading-state indicators;
- progress persistence;
- empty/error states designed as first-class screens.

### Reading

- distraction-free reading view;
- portrait and landscape;
- semantic position preservation after layout changes;
- table of contents;
- bookmarks;
- in-book search;
- finished-book state;
- restore to the previous reading position.

### Typography

Five built-in presets are planned:

**Spacious · Comfortable · Standard · Compact · Dense**

Each preset controls font size, line spacing and margins. Manual adjustment switches the book to **Custom**. Settings can be global or overridden per book.

The current reading typeface target is **Noto Sans**.

### Local file management

The first management workflow is intentionally local:

- upload one or many books from a browser on the same network;
- drag-and-drop transfer;
- view storage usage and free space;
- inspect and correct metadata;
- replace or remove covers;
- safely delete or replace book files;
- show import progress and errors;
- reset progress or per-book settings.

Reading itself does not depend on this web interface.

### Wi-Fi

Planned Wi-Fi behavior includes:

- saved networks;
- explicit trusted/preferred networks;
- deterministic reconnect behavior;
- password entry using the shared on-device keyboard;
- session-scoped local web management;
- no cloud account requirement.

### Sleep and power

- static sleep screens suitable for e-paper;
- optional book-cover sleep screen;
- no live clock or animation while sleeping;
- auto-sleep suspended during writes/imports;
- battery thresholds and wake behavior validated on real hardware.

---

## Current limitations

ENKU is still in the pre-firmware stage, so several important decisions remain intentionally open:

- there is no production reader firmware yet;
- supported book formats are not finalized;
- the parser and rendering stack are not selected;
- partial-refresh behavior has not yet been measured on the real panel;
- battery model and real-world runtime are not finalized;
- physical button mapping and wake behavior still require hardware testing;
- the enclosure has not been dimensioned from the production board yet;
- current EN design boards are design previews, not screenshots from running firmware.

These are active development items, not hidden assumptions.

## Firmware direction

Firmware has not yet been published as a working reader. The planned implementation is split into clear subsystems so the project can evolve without coupling the UI directly to storage or parser internals.

### Core layers

1. **Hardware abstraction**
   - display driver and refresh policy;
   - buttons and input events;
   - storage;
   - Wi-Fi;
   - battery/power state.

2. **Application state**
   - library index;
   - current book;
   - reading position;
   - bookmarks;
   - settings;
   - network state.

3. **Reader engine**
   - format parsing;
   - text layout and pagination;
   - typography presets;
   - semantic position mapping;
   - search and table of contents.

4. **UI system**
   - reusable status bar, headers, rows, buttons and dialogs;
   - focus navigation;
   - portrait/landscape layout;
   - localization;
   - e-paper-aware redraw strategy.

5. **Local management service**
   - upload/import;
   - metadata operations;
   - storage status;
   - safe replace/delete transactions.

The exact framework and parser stack will be chosen after display bring-up, memory profiling and real-device tests.

---

## Enclosure development

The enclosure is a separate project track, not an afterthought.

The plan is to develop the case in stages:

1. **Mechanical verification**
   - measure the production board;
   - verify display stack height, connectors, controls and battery space;
   - establish safe internal clearances.

2. **Functional prototype**
   - simple printable shell;
   - reliable access to controls and charging;
   - battery retention;
   - serviceable assembly.

3. **Ergonomic iteration**
   - comfortable grip;
   - practical button placement;
   - reduced thickness where mechanically safe;
   - portrait and landscape handling.

4. **Production-ready open model**
   - printable case files;
   - screw/assembly specification;
   - tolerances and print notes;
   - alternative battery/case variants where useful.

No final mechanical dimensions will be published as authoritative until the actual hardware is measured.

---

## Project roadmap

**At a glance:**  
**Design system → Hardware bring-up → Reader MVP → Connectivity → Enclosure → Open release**

### Phase 1 — Product and UI foundation

- [x] Define ENKU product direction
- [x] Select Waveshare ESP32-S3-ePaper-3.97 as reference platform
- [x] Define non-touch interaction model
- [x] Build reusable UI foundations and component rules
- [x] Audit the initial screen set
- [x] Define EN as the canonical UI language
- [ ] Finish canonical EN review boards
- [ ] Complete storage/import and Wi-Fi screen revisions

### Phase 2 — Hardware bring-up

- [ ] Verify board revision and physical controls
- [ ] Bring up the e-paper panel
- [ ] Characterize full/partial refresh behavior and ghosting
- [ ] Verify microSD and persistent storage
- [ ] Measure battery behavior and sleep current
- [ ] Validate wake/power-off behavior
- [ ] Confirm typography rendering and memory use

### Phase 3 — Reader MVP

- [ ] Select initial book format(s)
- [ ] Implement parser and text layout
- [ ] Implement Library grid/list navigation
- [ ] Implement reading position and progress persistence
- [ ] Implement typography presets and Custom mode
- [ ] Add bookmarks and table of contents
- [ ] Add in-book search
- [ ] Add sleep/restore flow

### Phase 4 — Import and connectivity

- [ ] microSD import
- [ ] validation and duplicate handling
- [ ] local Wi-Fi upload
- [ ] browser-based library management
- [ ] metadata and cover editing
- [ ] transactional replace/delete behavior
- [ ] interruption and recovery handling

### Phase 5 — Enclosure

- [ ] Measure final hardware
- [ ] Build first functional case prototype
- [ ] Test battery placement and assembly
- [ ] Refine ergonomics
- [ ] Publish printable enclosure files and assembly notes

### Phase 6 — Release quality

- [ ] Complete EN/RU localization
- [ ] Add PL/DE/FR/ES/IT UI translations
- [ ] Run portrait/landscape layout validation
- [ ] Test malformed books, missing storage and interrupted transfers
- [ ] Validate reboot and power-loss recovery
- [ ] Publish reproducible build instructions
- [ ] Select final code/design licenses
- [ ] Tag the first public reader release

A more detailed implementation roadmap lives in [ROADMAP.md](ROADMAP.md).

---

## UI and design system

The UI source of truth is being developed in Figma and documented in this repository.

Key rules:

- non-touch interaction;
- focus is not the same as selection;
- both **800 × 480 landscape** and **480 × 800 portrait** are supported;
- 24 px standard screen inset;
- 48 px baseline button/input height;
- safe default focus for destructive dialogs;
- English is the canonical source language;
- e-paper refresh limitations are treated as product constraints.

Useful documents:

- [Project status](docs/status.md)
- [Architecture](docs/architecture.md)
- [Hardware baseline](docs/hardware.md)
- [BOM status](docs/bom.md)
- [UI specification](docs/ui-spec.md)
- [UI audit](docs/ui-audit.md)
- [UI revision plan](docs/ui-revision-plan.md)
- [Screen inventory](docs/screens.md)
- [Navigation model](docs/navigation.md)

## Repository structure

```text
assets/
  branding/         ENKU identity assets

design/
  screens/          historical and current UI boards
  renders/          project diagrams and visual references

docs/
  architecture.md    system architecture
  hardware.md        verified/planned hardware baseline
  bom.md             parts/BOM status and sourcing priorities
  status.md          current project status
  UI and interaction specifications

README.md           project overview
ROADMAP.md          implementation roadmap
NOTICE.md           third-party asset and release notes
```

Firmware, hardware and enclosure directories will be added when those workstreams contain verified source material.

## Localization

**English is the canonical/source UI language.**

Planned first localization wave:

- English
- Russian
- Polish
- German
- French
- Spanish
- Italian

The interface language is independent from book content and keyboard input mode.


## Open-source status

ENKU is intended to be released as an open-source project.

The final code and design licenses have **not yet been selected**, so this repository should not be interpreted as granting a license to redistribute all included material today. Third-party fonts/icons and historical mockup assets are being reviewed separately.

See [NOTICE.md](NOTICE.md).

---

<p align="center">
  <strong>ENKU is being built as a real reading device: small, focused, open and understandable.</strong>
</p>
