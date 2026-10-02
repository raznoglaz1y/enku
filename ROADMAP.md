# ENKU Roadmap

This roadmap tracks the path from the current product/UI definition to a reproducible open-source e-reader release.

ENKU is still pre-firmware. Hardware-dependent items remain provisional until they are verified on the real Waveshare ESP32-S3-ePaper-3.97 board.

## 1. Product foundation

- [x] Define ENKU as a focused open-source e-reader project
- [x] Select Waveshare ESP32-S3-ePaper-3.97 as the reference platform
- [x] Define portrait and landscape as first-class orientations
- [x] Define a non-touch, physical-control interaction model
- [x] Define English as the canonical UI/source language
- [x] Define first localization wave: EN/RU/PL/DE/FR/ES/IT
- [x] Select Noto Sans as the current reading typography target
- [x] Define five reading presets: Spacious, Comfortable, Standard, Compact, Dense
- [x] Define offline-first behavior and optional Wi-Fi management
- [x] Define the local browser-based library-management scope

## 2. UI / UX system

- [x] Audit the initial 58-screen RU design baseline
- [x] Document focus vs selected vs active behavior
- [x] Define core dimensions, spacing, headers, rows and buttons
- [x] Define e-paper-aware interaction principles
- [x] Define Library state persistence
- [x] Define Reading Menu, Quick Aa, typography and search behavior
- [x] Define Storage/Import behavior
- [x] Define Wi-Fi and local management behavior
- [x] Define Sleep and Power principles
- [x] Build reusable Figma foundations and shared components
- [ ] Finish canonical EN review boards
- [ ] Finish Storage/Import board revisions
- [ ] Finish Wi-Fi board revisions
- [ ] Stress-test revised EN boards with long PL/DE/FR strings
- [ ] Validate localization placeholders, plural variants and missing-key fallback
- [ ] Replace temporary EN SVG boards with approved PNG review boards
- [ ] Synchronize screen index and documentation with the approved Figma baseline

## 3. Hardware bring-up

- [ ] Receive and identify the exact production board revision
- [ ] Measure the board and display stack
- [x] Verify control mapping against current official Waveshare source
- [ ] Physically verify onboard controls and GPIO behavior on the received board
- [ ] Verify microSD behavior and usable filesystem options
- [ ] Bring up the e-paper panel from a minimal firmware project
- [ ] Measure full-refresh timing
- [ ] Test available partial-refresh behavior
- [ ] Characterize ghosting across realistic reader screens
- [ ] Validate Noto Sans rendering and memory footprint
- [x] Define product-level power states: Active, DisplayIdle, Suspended, PoweredOff
- [ ] Measure active, panel-idle, candidate-suspend and powered-off current
- [ ] Verify battery charging behavior
- [ ] Select and validate the final battery pack
- [x] Verify current vendor PMU shutdown and panel-sleep behavior from official source
- [ ] Verify actual suspend/wake sources and hard power-off semantics on hardware
- [ ] Document the verified hardware baseline

## 4. Firmware foundation

- [x] Create the framework-neutral firmware project structure
- [ ] Add reproducible build instructions
- [x] Define initial core/service interface boundaries
- [ ] Implement hardware abstraction for display, input, storage and power
- [x] Define reader runtime/state-machine behavior
- [x] Define staged boot/startup and restore architecture
- [ ] Implement boot coordinator / stage state machine
- [ ] Instrument startup stage timings
- [ ] Implement application state model
- [x] Define persistence backend and crash-recovery model
- [ ] Implement CBOR persistence codec and generation recovery
- [ ] Implement persistent settings storage
- [ ] Implement reusable UI component layer
- [x] Define physical-to-logical input mapping and press semantics
- [ ] Implement deterministic focus navigation
- [ ] Implement portrait/landscape layout switching
- [x] Define localization key/fallback/plural architecture
- [ ] Add canonical English string catalog
- [ ] Add PL/DE/FR/ES/IT/RU translation catalogs
- [ ] Implement generated runtime localization tables
- [ ] Implement e-paper redraw scheduling / refresh policy
- [x] Define structured error/logging/recovery architecture
- [ ] Implement bounded structured log ring
- [ ] Implement persistent critical-error/reboot summary
- [ ] Implement boot-loop detection and Recovery Mode
- [ ] Add diagnostic export/support bundle
- [ ] Add structured logging and debug diagnostics

## 5. Library and storage

- [x] Define the initial supported book format set: EPUB, FB2 and TXT; PDF deferred
- [x] Define Library/storage data model and transactional import semantics
- [ ] Implement library indexing
- [x] Define metadata normalization and fallback rules
- [x] Define shared parser abstraction for EPUB, FB2 and TXT
- [ ] Implement metadata extraction
- [ ] Implement cover extraction and fallback handling
- [ ] Implement Grid view
- [ ] Implement List view
- [ ] Implement filter and sort
- [ ] Implement title/author search
- [ ] Persist Library focus, view, filter and sort state
- [ ] Implement safe delete
- [x] Define transactional replace behavior
- [ ] Implement transactional replace
- [x] Define duplicate detection stronger than filename matching
- [ ] Implement duplicate detection
- [x] Define temporary-file cleanup/recovery behavior
- [ ] Implement temporary-file cleanup after failed imports

## 6. Reader engine

- [x] Define format-independent normalized document model
- [ ] Implement EPUB parser
- [ ] Implement FB2 parser
- [ ] Implement TXT reader/parser
- [x] Define semantic reading-position representation
- [x] Define pagination, wrapping and rendering model
- [ ] Implement text layout and pagination
- [x] Define portrait/landscape repagination behavior
- [ ] Implement portrait/landscape repagination while preserving position
- [x] Define debounced per-book progress persistence layout
- [ ] Implement reading progress persistence
- [ ] Implement five typography presets
- [ ] Implement Custom typography
- [ ] Implement global and per-book settings
- [ ] Implement bookmarks
- [ ] Implement table of contents
- [ ] Implement in-book search
- [ ] Implement search match navigation/highlighting
- [ ] Implement finished-book state
- [x] Define sleep/wake and recovery state behavior
- [ ] Restore exact logical reading position after reboot/sleep

## 7. Wi-Fi and local management

- [ ] Implement Wi-Fi scanning and connection
- [ ] Store credentials only after a successful connection
- [ ] Implement saved and trusted/preferred network states
- [ ] Define deterministic auto-connect priority
- [ ] Implement the shared on-device keyboard
- [ ] Implement session-scoped local web server
- [ ] Support multi-file upload / drag-and-drop
- [ ] Show storage/free-space information
- [ ] Show import status and errors
- [ ] Support metadata inspection/correction
- [ ] Support cover replacement/removal/restoration
- [ ] Support mark read/unread
- [ ] Support progress reset
- [ ] Support clearing per-book overrides
- [ ] Validate interrupted transfer recovery

## 8. Sleep and power management

- [x] Define Sleep / suspend / power-off architecture
- [ ] Implement manual Sleep
- [ ] Implement configurable auto-sleep
- [ ] Suspend auto-sleep during writes/imports/transfers
- [ ] Implement static sleep screen
- [ ] Implement optional book-cover sleep screen
- [ ] Verify wake-to-previous-position behavior
- [ ] Characterize battery percentage behavior
- [ ] Define verified low-battery thresholds
- [ ] Implement low-battery UI
- [ ] Validate hard power-off workflow

## 9. Enclosure and mechanical design

- [ ] Measure the real board, display, controls and connectors
- [ ] Establish internal keep-out zones and assembly clearances
- [ ] Select the final battery envelope
- [ ] Build the first functional 3D-printable shell
- [ ] Verify display retention and front protection
- [ ] Verify button access and tactile feel
- [ ] Verify charging access
- [ ] Verify battery retention and serviceability
- [ ] Refine grip and edge geometry
- [ ] Minimize thickness where mechanically safe
- [ ] Validate portrait and landscape ergonomics
- [ ] Define screws/fasteners and assembly sequence
- [ ] Publish printable case files
- [ ] Publish print settings/tolerance notes
- [ ] Consider alternate battery/case variants only after the baseline case is stable

## 10. Validation

- [ ] EN/RU UI validation
- [ ] PL/DE/FR/ES/IT localization validation
- [ ] Portrait/landscape validation
- [ ] Long strings and missing metadata
- [ ] Missing/unavailable storage
- [ ] Corrupt/unsupported book files
- [ ] Interrupted import
- [ ] Interrupted Wi-Fi upload
- [ ] Reboot during safe operations
- [ ] Power-loss recovery
- [ ] Large library behavior
- [ ] Long-book pagination/search behavior
- [ ] Ghosting and refresh stress tests
- [ ] Battery runtime test with realistic reading usage

## 11. Open-source release

- [ ] Select final firmware license
- [ ] Select final design/mechanical license
- [ ] Verify third-party asset licenses and attribution
- [ ] Add firmware build and flash documentation
- [ ] Add hardware assembly documentation
- [ ] Add enclosure assembly documentation
- [ ] Finalize supported-format documentation and known limitations
- [ ] Re-evaluate PDF support after the reflowable reader is stable
- [ ] Publish known limitations
- [ ] Publish reproducible release artifacts
- [ ] Tag the first public ENKU reader release

---

## Milestone view

**M1 — Design baseline**  
Canonical UI rules, components and screen set.

**M2 — Hardware prototype**  
Display, controls, storage and power verified on the real board.

**M3 — Reader MVP**  
Library + reading + persistence working offline.

**M4 — Connectivity**  
Wi-Fi, local upload and browser-based management.

**M5 — Mechanical prototype**  
Verified printable enclosure around the final hardware stack.

**M6 — Open release**  
Reproducible build, documentation, case files and first tagged version.
