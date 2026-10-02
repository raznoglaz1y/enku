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
    events.hpp
    app_state.hpp
  reader/
    reader_types.hpp
    reader_engine.hpp
  services/
    services.hpp
```

Implementation directories will be added as hardware and subsystem work begins.

Architecture references:

- [System architecture](../docs/architecture.md)
- [Input & physical controls](../docs/input-model.md)
- [Power, sleep & wake model](../docs/power-model.md)
- [Persistence backend](../docs/persistence-model.md)
- [Localization architecture](../docs/localization-model.md)
- [Firmware structure](../docs/firmware-structure.md)
- [Reader runtime](../docs/runtime-state-machine.md)
- [Pagination model](../docs/pagination-model.md)
- [Parser model](../docs/parser-model.md)
- [Storage model](../docs/storage-model.md)

The public source language and code/documentation language for firmware is English.
