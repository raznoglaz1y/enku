# ENKU Firmware

This directory contains the ENKU firmware architecture scaffold.

**Status:** the framework-neutral core is host-buildable and tested. The first board-specific target now exists under `platform/esp_idf/` for Waveshare ESP32-S3-ePaper-3.97 storage bring-up. It is not yet a complete reader UI firmware.

## Current scaffold

```text
include/enku/
  core/
    types.hpp
    input.hpp
    power.hpp
    persistence.hpp
    localization.hpp
    settings.hpp
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
    library_runtime.hpp
    settings_runtime.hpp
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
    cbor_settings_service.hpp
    book_import_service.hpp
    staged_book_import_service.hpp
    book_delete_service.hpp
    book_file_store.hpp
    posix_book_file_store.hpp
    stored_book_source_service.hpp
    posix_state_file_store.hpp
```

The first framework-neutral implementations now include the TXT parser, paginator, DocumentReaderEngine, ReaderSession, reader runtime lifecycle (open/page/back/finished), and the first concrete A/B CBOR per-book checkpoint backend with CRC32 recovery, plus a POSIX filesystem adapter and host-side disk integration tests under `tests/`. A first ESP-IDF SDMMC/FAT filesystem adapter now exists under `platform/esp_idf/`, together with a minimal `app_main` storage smoke test. Display/input/power integration remains the next board bring-up work.

Architecture references:

- [System architecture](../docs/architecture.md)
- [Input & physical controls](../docs/input-model.md)
- [Power, sleep & wake model](../docs/power-model.md)
- [Persistence backend](../docs/persistence-model.md)
- [Global settings model](../docs/settings-model.md)
- [Localization architecture](../docs/localization-model.md)
- [Errors, logging & diagnostics](../docs/diagnostics-model.md)
- [Boot & startup architecture](../docs/boot-model.md)
- [Refresh Manager & e-paper update policy](../docs/refresh-model.md)
- [Firmware structure](../docs/firmware-structure.md)
- [Reader runtime](../docs/runtime-state-machine.md)
- [Pagination model](../docs/pagination-model.md)
- [Pagination MVP](../docs/pagination-mvp.md)
- [Reader Engine MVP](../docs/reader-engine-mvp.md)
- [Reader Session MVP](../docs/reader-session-mvp.md)
- [Reader Runtime Integration MVP](../docs/reader-runtime-mvp.md)
- [Book Loader / Document Provider MVP](../docs/book-loader-mvp.md)
- [Parser model](../docs/parser-model.md)
- [Storage model](../docs/storage-model.md)
- [Library data model & service interface](../docs/library-model.md)

The public source language and code/documentation language for firmware is English.


## Host build and tests

The framework-neutral firmware modules can be compiled and tested on a desktop host with CMake:

```bash
cmake -S firmware -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The host suite currently covers:

- TXT parsing;
- text pagination;
- Reader Engine;
- ReaderSession navigation;
- Book Loader;
- Reader runtime open/page/back/restore lifecycle;
- Library runtime filtering, sorting, search, focus navigation, focused-book open, staged-import flow and transactional delete;
- CBOR A/B checkpoint generation and corruption recovery;
- persistent CBOR Library index, queries, sorting, paging and summary updates;
- A/B global settings persistence with safe-default recovery and cold-boot application;
- TXT import pipeline with format detection, parse validation, content fingerprinting and duplicate rejection;
- transactional staged-file import from `/system/tmp` into canonical `/books/book-<id>.<ext>`;
- storage startup recovery that loads the Library, cleans stale committed tmp artifacts, and preserves ambiguous uploads;
- persisted last-safe app context and automatic cold-boot restore into Reading at the saved semantic checkpoint;
- Sleep checkpointing plus fast context-only Wake restore without repeating the full storage recovery path;
- graceful Power Off persistence before the platform PMU shutdown request;
- persistent boot-loop protection that enters Recovery before a third consecutive incomplete cold boot can auto-restore again;
- rollback of the final book file when Library commit fails;
- transactional book deletion with Library/source/checkpoint/context cleanup and rollback;
- POSIX filesystem persistence for book files and checkpoints.

GitHub Actions runs the same host suite on pushes to `main` and on pull requests.


## ESP-IDF board target

The first Waveshare ESP32-S3-ePaper-3.97 target lives at:

```text
firmware/platform/esp_idf/
```

It currently provides:

- board pin constants for TF, e-paper and navigation controls;
- 4-bit SDMMC FAT32 mount at `/sdcard`;
- creation of ENKU `/books` and `/system/*` directories;
- `EspIdfFileStore` implementing both `StateFileStore` and `BookFileStore`;
- path-traversal rejection;
- a serial storage smoke test in `app_main`;
- a build-verified SSD1677 800×480 monochrome display driver with full, fast and partial refresh modes;
- a PSRAM-backed ENKU display smoke sequence exercising all three refresh paths;
- active-low GPIO button polling with debounce, long-press/repeat generation, screen-aware logical action mapping and runtime dispatch into Library/Reader/power flows;
- AXP2101 power telemetry and shutdown/light-sleep platform service;
- mono framebuffer boundary and concrete e-paper RefreshService with full/fast/partial routing;
- unified ESP-IDF platform composition root owning storage, display, refresh, power and buttons;
- application storage runtime composition for settings, Library, import/delete, checkpoints, boot-loop state and startup recovery;
- FreeType-backed Noto Sans text measurement and monochrome glyph rendering for the Reader pipeline.

See [ESP-IDF platform bring-up](platform/esp_idf/README.md).


The board target is compiled separately in GitHub Actions against ESP-IDF v5.5.5. The first successful platform build covers both SDMMC storage and SSD1677 full-refresh code.
