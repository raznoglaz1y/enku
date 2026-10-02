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
      events.hpp
      app_state.hpp
    reader/
      reader_types.hpp
      reader_engine.hpp
    services/
      services.hpp

  src/
    core/
    reader/
    parsers/
      epub/
      fb2/
      txt/
    storage/
    ui/
    platform/
    services/
```

Only framework-independent interfaces are committed at this stage.

## 2. Module ownership

### core

Owns product-level state and events.

It must not depend on:

- e-paper driver code;
- filesystem implementation details;
- EPUB/FB2/TXT parser internals;
- Wi-Fi implementation details.

### reader

Owns:

- semantic positions;
- normalized reader requests/results;
- pagination contract;
- Reader Engine interface.

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
- parser libraries;
- persistence backend;
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
8. implement TXT parser first as the simplest Reader Engine path;
9. validate pagination on real panel;
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
