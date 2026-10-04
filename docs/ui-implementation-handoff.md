# ENKU UI implementation handoff

**Status:** firmware foundation ready for final EN-first UI design.

This document is the handoff boundary between the platform/runtime work and the final Figma pass. It describes the states the current firmware can already produce and the boards/components that now need a final visual contract before placeholder rendering should be expanded further.

## 1. Runtime screens already represented

The persisted/runtime screen model currently contains:

- Boot
- Library
- Book Opening
- Reading
- Reader Overlay
- Search
- Settings
- Import / Transfer
- Sleep
- Error / Recovery
- Book Details
- Book Finished
- Contents / Bookmarks
- About Book
- Reading Settings
- Display Settings
- Locale / Language Settings
- About Device
- Power Off confirmation
- Wi-Fi Settings

Do not renumber existing persisted Screen values.

## 2. Boards needed now

### Shared system components

Finalize these before individual screens:

- status bar in portrait and landscape;
- standard header/back pattern;
- focused row;
- selected value vs focused value;
- primary/secondary/destructive actions;
- tabs;
- scroll indicator;
- modal/confirmation container;
- empty-state pattern;
- warning/error banner;
- on-screen keyboard;
- long-text truncation/wrapping;
- disabled/unavailable action state.

### Library

Final EN boards/states needed:

- Grid Library;
- List Library;
- explicit focused book in both views;
- Empty Library;
- Search + keyboard;
- No search results;
- Book Details;
- Delete confirmation;
- Restart reading confirmation;
- SD unavailable;
- SD unreadable/error;
- SD verification in progress;
- per-book availability states:
  - normal/progress;
  - CHECKING (not yet fingerprint-verified after remount);
  - OFFLINE (verified missing/corrupt/mismatched).

For SD verification the runtime exposes checked/total progress. The UI may present copy such as “Checking library 37/214”, but final wording belongs to the EN localization/design pass.

### Reader

Finalize:

- clean Reading page;
- Reader Menu/Overlay;
- Quick typography preview;
- full Reading/Typography Settings;
- Contents tab;
- Bookmarks tab;
- empty Contents;
- empty Bookmarks;
- in-book Search + keyboard;
- search result/match navigation;
- About Book;
- Book Finished.

Reading position is semantic and survives typography/orientation changes.

### Settings / device

Finalize:

- Settings root;
- Reading;
- Display;
- Wi-Fi;
- Language;
- Storage;
- Sleep;
- About Device;
- Power Off confirmation.

The Settings root contract is:
Reading / Display / Wi-Fi / Language / Storage / Sleep / About / Power Off.

### Import / transfer

This is now UI-blocking work. Finalize the product flow for:

- Storage overview;
- import method selection;
- local/microSD file browser;
- validation;
- duplicate/replace decision;
- import progress;
- import complete;
- batch summary;
- cancel confirmation;
- interrupted/failed transfer;
- insufficient space.

microSD/local files are the guaranteed MVP path. Wi-Fi transfer has firmware infrastructure and should be designed as a local-network workflow. USB transfer remains reference-only until transport architecture is explicitly committed.

### Error / Recovery

Finalize a reusable error presentation rather than one unique screen per internal error.

Product-facing states need at least:

- book cannot be opened;
- storage unavailable;
- storage unreadable;
- import interrupted;
- insufficient space;
- Wi-Fi authentication/connection failure;
- suspend failed;
- generic recoverable operation failure;
- Recovery/Safe Mode.

The firmware diagnostics model separates raw diagnostics from user copy. Normal UI must show: what failed, whether data is safe, and the valid recovery actions. Raw subsystem errors do not belong in ordinary UI.

Recovery actions supported by the architecture include:
Retry / Back / Return to Library / Reinsert storage / Reconnect / Reboot / Power Off.

Destructive actions must never receive default focus.

## 3. Lifecycle states the UI must account for

- Reading works offline.
- Missing SD does not prevent normal boot to Library.
- Removing SD while reading closes the live reader safely and returns to Library.
- On remount, books are fail-closed until their full fingerprint is verified.
- Remount verification is incremental; UI remains responsive.
- A book pending verification is distinct from a verified missing/corrupt book.
- Import/transfer activity blocks Sleep.
- Failed suspend returns to the previous active screen and restores network connectivity best-effort.
- Wake restores the previous Library/Reading context; Wi-Fi recovery is best-effort and must not block reading.
- Power Off confirmation defaults to Cancel.
- Network/PMU failures are degradable where possible and must not impersonate a dead reader.

## 4. Do not finalize from mockups yet

These require physical hardware validation before exact UI promises are frozen:

- exact battery percentage accuracy and low-battery threshold;
- final sleep timeout choices;
- e-paper refresh-policy user options;
- exact physical control labels/help;
- true power-off/wake wording for the final board;
- USB transfer;
- any sensor-driven orientation behavior.

Design these as replaceable/parameterized states, not hard-coded product promises.

## 5. Design rules already fixed

- English is the canonical/source UI language.
- Initial localization target: EN / RU / PL / DE / FR / ES / IT.
- Noto Sans is the reading/UI typography baseline.
- Phosphor Icons are the icon baseline.
- Portrait and landscape share behavior; layout may change.
- Focus and selected state are visually distinct.
- Long lists scroll while retaining deterministic physical-button focus.
- Destructive confirmations default to Cancel.
- Sleep screens are static.
- Search keyboard/input mode is independent from interface language.
- Reading presets: Spacious / Comfortable / Standard / Compact / Dense; manual edits become Custom.

## 6. Firmware boundary

From this point, expanding placeholder renderer layouts is intentionally paused unless required for a bug fix or hardware bring-up.

The next major UI implementation pass should consume approved final EN Figma boards/components. Runtime, persistence, storage recovery, reader lifecycle, network lifecycle and diagnostics can continue independently, but new product-facing screen composition should follow the final design system.

This is the point at which final UI mockups are no longer optional reference material: they are the input needed for the next implementation phase.
