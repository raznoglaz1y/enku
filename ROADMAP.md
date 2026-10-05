# ENKU Roadmap

This roadmap tracks the path from the current product/UI definition to a reproducible, repairable reader release with staged public source/design releases.

ENKU is in active firmware implementation and custom-hardware development. The Waveshare ESP32-S3-ePaper-3.97 remains the reference bring-up platform, while the dedicated ENKU R0.1 mainboard is in late routing / pre-production verification. Hardware-dependent claims remain provisional until physically validated.

## 1. Product foundation

- [x] Define ENKU as a focused, repairable, local-first e-reader project
- [x] Select Waveshare ESP32-S3-ePaper-3.97 as the reference platform
- [x] Define portrait and landscape as first-class orientations
- [x] Keep 3.97″ as the R0.1 pocket-reader target; larger formats are a separate future branch, not a replacement
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
- [ ] Measure region/full refresh escalation thresholds using Refresh Manager diagnostics
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
- [x] Integrate PageNext/PagePrevious events with ReaderSession and AppState
- [x] Integrate OpenBook/Back/Finished transitions
- [x] Implement concrete TXT book-loader/document-provider service
- [ ] Wire book-loader completion directly into runtime event dispatch
- [ ] Implement application state model
- [x] Define persistence backend and crash-recovery model
- [ ] Implement CBOR persistence codec and generation recovery
- [ ] Implement persistent settings storage
- [ ] Implement reusable UI component layer
- [x] Define physical-to-logical input mapping and press semantics
- [x] Define ambidextrous control requirement: Portrait RH / Portrait LH / Landscape RH / Landscape LH with logical remapping
- [ ] Implement deterministic focus navigation
- [ ] Implement portrait/landscape layout switching
- [x] Define localization key/fallback/plural architecture
- [ ] Add canonical English string catalog
- [ ] Add PL/DE/FR/ES/IT/RU translation catalogs
- [ ] Implement generated runtime localization tables
- [x] Define Refresh Manager, coalescing and ghosting-measurement architecture
- [ ] Implement render-plan generation
- [ ] Implement bounded Refresh Manager queue/coalescing
- [ ] Implement e-paper redraw scheduling / refresh policy
- [x] Define structured error/logging/recovery architecture
- [ ] Implement bounded structured log ring
- [ ] Implement persistent critical-error/reboot summary
- [ ] Implement boot-loop detection and Recovery Mode
- [ ] Add diagnostic export/support bundle
- [ ] Add structured logging and debug diagnostics

## 5. Library and storage

- [x] Define the initial supported book format set: EPUB, FB2 and TXT; PDF deferred
- [ ] Add Markdown as a lightweight supported reading format
- [x] Define Library/storage data model and transactional import semantics
- [x] Define Library records, browse/search query model and service boundary
- [ ] Implement LibraryService and index loading
- [ ] Implement library indexing
- [x] Define metadata normalization and fallback rules
- [x] Define shared parser abstraction for EPUB, FB2 and TXT
- [ ] Implement metadata extraction
- [ ] Implement cover extraction and fallback handling
- [x] Define bounded Library query paging and focus-by-book-id behavior
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
- [x] Add framework-neutral TXT parser MVP (UTF-8 validation, normalization, paragraph blocks)
- [ ] Integrate TXT parser with storage/Reader Engine and expand encoding support
- [x] Define semantic reading-position representation
- [x] Define pagination, wrapping and rendering model
- [x] Add first host-testable forward text-pagination MVP
- [ ] Add real font metrics and production text shaping/measurement
- [ ] Implement paragraph spacing, block styles and deterministic previous-page reconstruction
- [x] Implement first DocumentReaderEngine over TextPaginator
- [x] Add ReaderSession page history/cache and deterministic Previous
- [ ] Add bounded long-session checkpoint/history compaction
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

## 7. Connectivity and local management

- [ ] Implement Wi-Fi scanning and connection
- [ ] Store credentials only after a successful connection
- [ ] Implement saved and trusted/preferred network states
- [ ] Define deterministic auto-connect priority
- [ ] Implement the shared on-device keyboard
- [ ] Implement session-scoped local web server
- [ ] Implement USB Mass Storage / direct USB access to the microSD library
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
- [ ] Treat screen-to-body ratio / thin bezels as a mechanical KPI while preserving repairability
- [ ] Validate portrait and landscape ergonomics
- [ ] Validate left/right-handed ergonomics in both portrait and landscape
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

## 11. Public release

- [x] Select Apache-2.0 for original firmware/software
- [x] Select CC BY 4.0 for original project documentation
- [x] Select CERN-OHL-P-2.0 for future project-owned hardware/mechanical source
- [ ] Complete design-asset/third-party licensing audit
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

**M6 — Stable public release**  
Reproducible build, documentation, case files and first tagged version.


## 12. Custom mainboard / productization

- [x] Delayed-open commercialization policy fixed: ownership/repairability at launch; source/design files released selectively after maturity
- [x] Trademark separated from open hardware/software source
- [x] Pricing policy fixed: full COGS + sustainable margin, not raw-BOM pricing
- [x] Kickstarter rule fixed: no launch before a working physical prototype demonstrates marketed core features
- [x] Power cost/quality shortlist added: BQ25185, TPS2121, TPS63802 first-spin baseline, BQ51013C for Pro Wireless


- [x] Define portrait-first custom-mainboard direction
- [x] Define common Base / Cover / Pro / Pro Wireless architecture
- [x] Define Dock pogo charging and explicit DOCK_DETECT
- [x] Define configurable Dock Mode product concept
- [x] Add BMI270 motion/orientation target
- [x] Add Hall sensor / magnetic-cover target
- [x] Reserve Pro frontlight and Pro Wireless Qi paths
- [x] Raise warm adjustable frontlight to a production-candidate priority based on repeated user feedback; do not destabilize R0.1 routing to retrofit it
- [x] Define Base display candidate: GDEY0397T81P
- [x] Define Pro display candidate: GDEY0426T82-FL01C
- [x] Define multi-variant BOM ceilings and competitive stop limits
- [ ] Receive Good Display pricing / current reference circuitry for Base and Pro panels; SSD1677 family baseline documented
- [x] Select charger / power-path baseline: BQ25185; thermal validation still required
- [x] Select Dock power-mux quality baseline: TPS2121; discrete cost-down comparison still required
- [x] Select first-spin 3.3 V regulator: TPS63802DLAR after simultaneous-load margin review
- [x] Select exact microSD connector: Hirose DM3AT-SF-PEJM5; edge orientation corrected in PCB
- [ ] Select final low-noise page tact switches
- [x] Select Pro frontlight driver architecture: single TPS923610 boost + warm/cool selection
- [x] Freeze Pro frontlight prototype component baseline: TPS923610 + 10 µH + 15 Ω + low-side selectors
- [ ] Verify FL0426-S01C pin mapping and validate warm/cool current/blending on hardware
- [ ] Validate Qi coil / receiver / ferrite stack for Pro Wireless
- [ ] Complete per-variant populated / DNP BOM
- [x] Define PCBWay prototype path: fabrication + SMT assembly package, DFM gates and sponsored-vs-unsponsored cost separation
- [ ] Obtain normal unsponsored PCBWay / assembly quotes at 10 / 25 / 50 / 100 units
- [ ] Re-run competitive pricing after real quotes
- [x] Verify current Good Display product/spec/drawing availability for GDEY0397T81P and GDEY0426T82-FL01C
- [ ] Verify current GDEY0397T81P / GDEY0426T82-FL01C FPC pin tables and Pro 6-pin frontlight pinout directly from the drawings
- [x] Define schematic block/net baseline for USB/Dock/charge/3V3/ESP32/SD/EPD/IMU/Hall/frontlight/Qi/debug
- [x] Freeze SSD1677 reference electrical values and R0.1 EPD-HV sourcing candidates
- [x] Re-freeze R0.1 EPD-HV inductor from current 3.97-inch reference: TYS5040100M-10, 10 µH; 47 µH retained only as same-footprint lab alternate
- [x] Close R0.1 connector/frontlight-remap/EPD-HV placement blockers
- [x] Start real KiCad R0.1 worktree with 54 × 94 mm portrait four-layer PCB shell
- [x] Start PCB placement baseline with dual-contact EPD/FL connectors, EPD boost parts and Pro frontlight zone
- [x] Start hierarchical KiCad R0.1 schematic capture (Power / MCU+IO / EPD-HV / Frontlight / Connectors)
- [x] Replace core placement placeholders with verified local footprints: TPS2121 / BQ25185 / BMI270 / TPS923610 / EPD+FL Hirose FPC
- [x] Add project-local ENKU.pretty library and gate exact pad sets in CI
- [ ] Capture the first complete KiCad schematic and run ERC
- [ ] Freeze schematic only after ERC + exact footprint review

### Cost ceilings

- Electronics Kit: <= PLN 155
- Base: <= PLN 180
- Cover: <= PLN 200
- Pro: <= PLN 230
- Pro Wireless: <= PLN 260
- Dock: <= PLN 40

If supplier / assembly quotes materially exceed these ceilings, cost-down happens before final routing.

## 13. Software priorities after competitive review

Priority order:

1. robust EPUB / FB2 / TXT reader;
2. Library + persistence;
3. typography;
4. bookmarks / TOC / in-book search;
5. local browser-based library manager;
6. Dock Mode;
7. offline dictionary;
8. PDF only after the reflowable reader is stable.

Explicitly out of R0.x scope unless strategy changes:
- DRM/store integration;
- audio/TTS;
- waterproofing.

## 14. Possible future product directions

Community feedback to evaluate after the 3.97-inch R0.1 baseline is validated:

- [ ] Explore a mid-size ENKU variant around 4.7–5.0 inches as a possible sweet spot between pocketable ~3.9-inch readers and standard 6-inch devices.
- [ ] Reassess market differentiation of the 3.97-inch format as XTEink/XTE-class devices and clones become more common.
- [ ] Compare ergonomics, battery envelope, enclosure thickness, display availability, BOM impact and PCB reuse between 3.97-inch and 4.7–5.0-inch variants.
- [ ] Treat repeated community requests for a mid-size reader as a product signal, but do not change R0.1 scope before the current 3.97-inch hardware is completed and validated.
- [ ] Explore a narrow phone-sized 5.5–5.9-inch ENKU concept as a separate future branch, using PocketBook Q as market validation for demand in the compact-but-larger-than-4-inch category.
- [ ] Compare a 5.5–5.9-inch phone-like layout against the 4.7–5.0-inch compact-reader concept on ergonomics, BOM, battery envelope, display sourcing, enclosure width, controls and differentiation.



## 13. Community-derived product decisions — October 2026

The first broad r/ereader feedback review produced several product signals strong enough to record explicitly rather than leave as feature requests.

- [x] Hardware differentiation is the primary positioning requirement: ENKU must justify itself through the complete physical product, not only through open-source firmware.
- [x] Keep R0.1 at 3.97″ as a genuinely pocketable reader.
- [ ] Research a separate compact 4.7–5.2″ reader, preferably a book-like 4:3 format, after the Pocket baseline is proven.
- [x] Keep repairability as a core constraint: screws/fasteners, serviceable battery, documented connectors, published PCB/BOM/schematics and printable enclosure.
- [ ] Evaluate CrossPoint compatibility / port feasibility. ENKU OS remains the default, but open hardware should not unnecessarily lock users to one open firmware stack.
- [ ] Treat warm adjustable frontlight as a production-candidate priority; validate optics, power, thermals and BOM before committing it to a shipping variant.
- [ ] Implement USB Mass Storage / direct microSD access alongside Wi-Fi transfer.
- [ ] Add Markdown reading support.
- [ ] Validate four physical-control configurations: portrait RH/LH and landscape RH/LH.
- [ ] Optimize screen-to-body ratio and bezel width without sacrificing protection, assembly or repairability.
- [ ] Track battery life as a product KPI, not only an electrical measurement.
- [ ] Investigate the open 4.26″ Silkscreen project as an engineering reference / potential collaboration lead.
- [ ] Keep BLE keyboard/writerdeck support, modular accessories and pen input in exploratory backlog.
- [x] Keep Qi outside the Base requirement; wireless charging remains an optional higher-tier path.
- [x] Position ENKU as a complete user-owned reader platform: pocketable, repairable, offline-first, no account and no cloud requirement; source/design releases follow maturity.

