# ENKU Global Settings Model

This document defines the Reader v1 global settings domain and its persistence/runtime boundaries.

## 1. Scope

Global settings contain durable user preferences that should survive reboot and apply across books.

They do not contain:

- current book;
- semantic reading position;
- Library focus;
- active search text;
- current query offset;
- network credentials;
- transient overlays or render state.

Those belong to their own runtime/persistence domains.

## 2. Reader v1 settings

The current `GlobalSettings` record contains:

- UI locale;
- portrait/landscape orientation;
- Library Grid/List view;
- Library reading-state filter;
- Library sort mode;
- Library sort direction;
- reading typography preset;
- default font size;
- default line spacing;
- default margins;
- Wi-Fi product policy.

## 3. Typography presets

Stable preset ids are:

- Spacious;
- Comfortable;
- Standard;
- Compact;
- Dense;
- Custom.

The persisted value is the enum/id, never the localized label.

Custom typography persists explicit font size, line spacing and margins.

Per-book overrides are not global settings and remain a future per-book-state extension.

## 4. Wi-Fi policy

Reader v1 product policy values are:

- Off;
- Manual;
- AutoConnectTrusted.

This field controls desired product behavior only.

Trusted-network credentials/keys may later live in the platform secure storage mechanism and are intentionally not stored inside the ordinary settings CBOR record.

## 5. Persistence

Global settings use:

```text
/system/settings.a.cbor
/system/settings.b.cbor
```

`CborSettingsService` uses the same generation + CRC32 recovery pattern as other critical ENKU records.

The settings record is independent from:

- Library index;
- app restore context;
- boot-loop marker;
- per-book checkpoints.

## 6. Corruption policy

Settings corruption is not Library corruption.

If both settings generations are missing:

1. create safe defaults;
2. persist them;
3. continue normal startup.

If both generations are structurally/CRC invalid:

1. discard the invalid preference state;
2. create and persist safe defaults;
3. continue normal startup.

Real filesystem I/O or no-space failures remain persistence failures and may force Recovery Mode because the firmware cannot establish a safe durable settings state.

## 7. Boot application

Cold storage startup applies settings before exposing Library as usable:

```text
Persistence stage
→ SettingsRuntimeController::loadAndApply()
→ CborSettingsService
→ apply to AppState
→ load Library
→ transaction recovery
→ first usable screen
```

This means orientation, locale, Library preferences, typography defaults and Wi-Fi policy are already available to later UI/platform initialization.

## 8. Runtime updates

Settings changes use typed events and `SettingsRuntimeController`.

Current persisted events include:

- LocaleChanged;
- OrientationChanged;
- LibraryViewChanged;
- LibraryFilterChanged;
- LibrarySortChanged;
- TypographyDefaultsChanged;
- WiFiPolicyChanged.

The controller updates AppState, commits the complete settings record, and rolls AppState back if persistence fails.

Library filter/sort/view changes are already connected to this persistence path through `LibraryRuntimeController`.

## 9. Safe defaults

Current framework-neutral defaults are:

```text
locale = English
orientation = Portrait
Library view = Grid
Library filter = All
Library sort = RecentlyOpened / Descending
reading preset = Standard
font size = 18
line spacing = 1.35
margins = 24
Wi-Fi policy = AutoConnectTrusted
```

Exact physical typography metrics can still be tuned after real-panel profiling; persisted schema semantics remain stable.

## 10. Decisions fixed by this document

- Global preferences use a dedicated A/B CBOR record.
- Persisted values are stable ids/enums, not localized labels.
- Settings corruption falls back to defaults rather than destroying or blocking Library data.
- Search/focus/current-book state is not global settings.
- Wi-Fi credentials are not stored in ordinary settings CBOR.
- Runtime settings changes persist atomically and roll back in-memory preference changes if commit fails.
