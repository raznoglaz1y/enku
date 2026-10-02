# ENKU Firmware

This directory contains the ENKU firmware architecture scaffold.

**Status:** interfaces and source layout only. This is **not yet a buildable reader firmware**.

The scaffold intentionally avoids choosing Arduino vs ESP-IDF until the real Waveshare ESP32-S3-ePaper-3.97 board is available for bring-up and profiling.

## Current scaffold

```text
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
  services/
    services.hpp
  storage/
    state_file_store.hpp
    cbor_reader_checkpoint.hpp
    cbor_library_service.hpp
    cbor_app_context_service.hpp
    book_import_service.hpp
    staged_book_import_service.hpp
    book_file_store.hpp
    posix_book_file_store.hpp
    stored_book_source_service.hpp
    posix_state_file_store.hpp
```

The first framework-neutral implementations now include the TXT parser, paginator, DocumentReaderEngine, ReaderSession, reader runtime lifecycle (open/page/back/finished), and the first concrete A/B CBOR per-book checkpoint backend with CRC32 recovery, plus a POSIX filesystem adapter and host-side disk integration tests under `tests/`. A board-specific ESP32 filesystem adapter will be added after bring-up.

Architecture references:

- [System architecture](../docs/architecture.md)
- [Input & physical controls](../docs/input-model.md)
- [Power, sleep & wake model](../docs/power-model.md)
- [Persistence backend](../docs/persistence-model.md)
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
- CBOR A/B checkpoint generation and corruption recovery;
- persistent CBOR Library index, queries, sorting, paging and summary updates;
- TXT import pipeline with format detection, parse validation, content fingerprinting and duplicate rejection;
- transactional staged-file import from `/system/tmp` into canonical `/books/book-<id>.<ext>`;
- storage startup recovery that loads the Library, cleans stale committed tmp artifacts, and preserves ambiguous uploads;
- persisted last-safe app context and automatic cold-boot restore into Reading at the saved semantic checkpoint;
- Sleep checkpointing plus fast context-only Wake restore without repeating the full storage recovery path;
- rollback of the final book file when Library commit fails;
- POSIX filesystem persistence for book files and checkpoints.

GitHub Actions runs the same host suite on pushes to `main` and on pull requests.
