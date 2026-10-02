# ENKU Firmware Project Structure

This document defines the initial firmware source layout and module boundaries.

The scaffold is intentionally **framework-neutral**. Board-specific build files, ESP-IDF/Arduino integration and hardware drivers will be added only after the target Waveshare board is in hand and the bring-up path is verified.

## 1. Directory layout

```text
firmware/
  README.md

  include/enku/
    core/
      types.hpp
      input.hpp
      power.hpp
      persistence.hpp
      localization.hpp
      diagnostics.hpp
      boot.hpp
      refresh.hpp
      library.hpp
      events.hpp
      app_state.hpp
    reader/
      document.hpp
      parser.hpp
      txt_parser.hpp
      pagination.hpp
      reader_types.hpp
      reader_engine.hpp
      document_reader_engine.hpp
      reader_session.hpp
      book_loader.hpp
    runtime/
      reader_runtime.hpp
      storage_startup.hpp
      boot_restore.hpp
      sleep_wake.hpp
      power_off.hpp
    services/
      services.hpp
    storage/
      state_file_store.hpp
      cbor_reader_checkpoint.hpp
      cbor_library_service.hpp
      cbor_app_context_service.hpp
      cbor_boot_loop_service.hpp
      book_import_service.hpp
      staged_book_import_service.hpp
      book_file_store.hpp
      posix_book_file_store.hpp
      stored_book_source_service.hpp
      posix_state_file_store.hpp

  src/
    core/
    runtime/
      reader_runtime.cpp
      storage_startup.cpp
      boot_restore.cpp
      sleep_wake.cpp
      power_off.cpp
    reader/
      text_paginator.cpp
      document_reader_engine.cpp
      reader_session.cpp
      book_loader.cpp
    parsers/
      epub/
      fb2/
      txt/
        txt_parser.cpp
    storage/
      cbor_reader_checkpoint.cpp
      cbor_library_service.cpp
      cbor_app_context_service.cpp
      cbor_boot_loop_service.cpp
      book_import_service.cpp
      staged_book_import_service.cpp
      stored_book_source_service.cpp
      posix_book_file_store.cpp
      posix_state_file_store.cpp
    ui/
    platform/
    services/
```

Framework-independent interfaces are committed at this stage, plus host-testable TXT parser, forward text-pagination, concrete Reader Engine and ReaderSession navigation implementations, and the first concrete per-book A/B CBOR checkpoint backend.

## 2. Module ownership

### core

Owns product-level state and events.

It must not depend on:

- e-paper driver code;
- filesystem implementation details;
- EPUB/FB2/TXT parser internals;
- Wi-Fi implementation details.

### boot

Owns:

- staged startup progression;
- boot mode selection (Normal / First Start / Recovery);
- first-usable-screen checkpoint;
- coordination of persistence/storage/display/input readiness;
- automatic restore eligibility.

The boot coordinator does not own low-level driver initialization details; it sequences platform/service capabilities and records observable boot stages.

### diagnostics

Owns:

- structured product error domains/severity/codes;
- centralized error-to-UI mapping;
- bounded runtime log records;
- critical/fatal failure summaries;
- boot-loop/recovery markers;
- Recovery/Safe Mode diagnostics.

Normal product code should return/report structured errors rather than expose raw driver/parser strings directly to UI.

### localization

Owns:

- stable semantic string keys;
- locale selection;
- English fallback;
- named placeholder substitution;
- plural-category selection;
- generated runtime locale tables.

Human-reviewable source catalogs live under `locales/`. The runtime representation is generated and must not require parsing source JSON files on device startup.

### refresh

Owns:

- render-plan/update classification;
- dirty-region metadata;
- bounded refresh queue;
- update coalescing and stale-work removal;
- full-refresh escalation policy;
- refresh diagnostics/statistics.

Refresh policy remains independent from raw panel-driver APIs. Hardware-specific capability mapping lives under `platform`.

### library

Owns:

- normalized Library records used by UI/runtime;
- Browse vs Search query semantics;
- All/New/Reading/Finished filtering;
- Title/Author/RecentlyOpened/RecentlyAdded sorting;
- bounded result paging;
- stable focus/lookup by `book_id`;
- Library summary updates independent from detailed per-book state.

The Library service does not expose CBOR or filesystem details to UI code.

### runtime

Owns:

- application-level coordination of typed events;
- ReaderSession invocation for page navigation;
- synchronization of visible Reader state into AppState;
- progress-dirty hand-off to persistence;
- generation of refresh requests after successful visible changes;
- Book Opening → Reading / failure transitions;
- Back-to-Library progress checkpoint coordination;
- Finished-state Library summary updates;
- startup coordination across Library persistence and staged import recovery;
- safe restoration of the last persisted Library/Reading context after reboot;
- Sleep/Wake orchestration across reader persistence, network shutdown and PowerService;
- graceful Power Off persistence before platform shutdown;
- persistent boot-loop marker management for cold-boot recovery gating.

The runtime layer does not manipulate semantic offsets directly.

### reader

Owns:

- semantic positions;
- normalized reader requests/results;
- pagination contract;
- Reader Engine interface;
- stateful ReaderSession navigation;
- deterministic visited-page history;
- disposable previous/current/next page working-set cache;
- normalized book-loader/document-provider ownership for parser → document → engine → session.

It consumes normalized document content rather than raw source-format structures.

### parsers

Format adapters:

- EPUB;
- FB2;
- TXT.

Each parser converts source files into the normalized document/parser model defined in `docs/parser-model.md`.

### storage

Owns:

- Library index;
- book identity/fingerprint;
- versioned CBOR persistence;
- A/B generation recovery;
- metadata persistence;
- progress;
- bookmarks;
- transactional import/replace;
- temporary-file recovery.

The Reader v1 persistence format is defined in `docs/persistence-model.md`.

### ui

Owns:

- screen composition;
- focus/navigation presentation;
- render plans;
- dialogs/overlays;
- localization presentation.

UI does not talk directly to GPIO, SD card or parser internals.

### platform

Owns hardware-specific implementations:

- display;
- input/buttons;
- storage transport/filesystem bridge;
- Wi-Fi;
- power/battery;
- sleep/wake;
- clock if required.

Raw board details stay here.

### services

Defines cross-cutting service contracts used by application/runtime code.

Examples:

- storage service;
- book source service;
- reader checkpoint service;
- display/refresh service;
- network service;
- power service;
- clock service;
- logging service.

## 3. Dependency direction

Preferred dependency direction:

```text
platform implementations
        ↓
service interfaces
        ↓
application/core runtime
   ↙              ↘
reader             ui
  ↓
parser/storage abstractions
```

Higher-level modules must not include board-specific headers.

## 4. Event-driven core

The runtime consumes typed events from `events.hpp`.

Examples include:

- input events;
- navigation events;
- book-open events;
- typography/orientation changes;
- Wi-Fi/storage events;
- power events.

The event model is deliberately separate from raw button GPIO transitions.

## 5. Application state

`AppState` is the authoritative product-level state container.

It stores logical state only.

It must not store:

- framebuffer pixels;
- raw display-driver handles;
- parser DOM trees;
- temporary layout buffers;
- GPIO numbers.

Transient rendering/cache data belongs to the relevant subsystem.

## 6. Reader interface

`ReaderEngine` works with semantic positions and viewport/typography inputs.

It does not expose EPUB/FB2/TXT-specific objects to UI code.

A valid page result carries:

- first semantic position;
- last semantic position;
- previous anchor;
- next anchor;
- progress;
- display-list/layout payload through an implementation-defined representation.

## 7. Service interfaces

Service contracts are narrow by design.

The application should request capabilities rather than reach directly into implementations.

Examples:

```text
StorageService.openBook(...)
PowerService.requestSleep()
NetworkService.connect(...)
RefreshService.submit(renderPlan)
```

Exact methods will evolve during implementation, but service boundaries are fixed.

## 8. What is intentionally not committed yet

Not selected yet:

- Arduino vs ESP-IDF as the final firmware framework;
- build system files;
- exact display library;
- EPUB/FB2 parser libraries;

- task/thread model;
- exact PSRAM allocation strategy.

These remain open until hardware bring-up and profiling.

## 9. First implementation sequence

Recommended order after hardware arrival:

1. minimal board build/flash;
2. display bring-up;
3. physical input event capture;
4. storage/microSD validation;
5. power/sleep measurements;
6. connect verified platform drivers to service interfaces;
7. implement AppState/event reducer;
8. integrate and profile the existing TXT parser MVP as the simplest Reader Engine path;
9. connect the existing paginator to real Noto Sans metrics and validate it on the panel;
10. add EPUB and FB2 adapters;
11. add Library persistence/import;
12. add Wi-Fi management.

## 10. Decisions fixed by this document

- Firmware is split by responsibility, not by screen.
- Hardware-specific code stays under `platform`.
- UI never directly owns parser/storage/hardware internals.
- Core state/events remain framework-independent.
- EPUB/FB2/TXT remain adapters behind common reader contracts.
- Current scaffold is intentionally not presented as working firmware.
