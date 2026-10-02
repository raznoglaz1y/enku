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
  services/
    services.hpp
```

The first framework-neutral implementations now include `src/parsers/txt/txt_parser.cpp`, `src/reader/text_paginator.cpp` and `src/reader/document_reader_engine.cpp` and `src/reader/reader_session.cpp`, with host-side assertion tests under `tests/`. Hardware-specific implementation directories will grow after board bring-up.

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
- [Parser model](../docs/parser-model.md)
- [Storage model](../docs/storage-model.md)
- [Library data model & service interface](../docs/library-model.md)

The public source language and code/documentation language for firmware is English.
