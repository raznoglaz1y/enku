# ENKU Reader Engine MVP

This document describes the first concrete Reader Engine implementation built on top of the normalized document model and TextPaginator.

## Current path

```text
TXT bytes
→ TxtParser
→ BookDocument
→ DocumentReaderEngine
→ TextPaginator
→ PageResult
```

This is the first complete host-testable Reader path from source text to a Reader-level page result.

## Responsibilities

The Reader Engine MVP:

- owns the bridge between a currently opened `BookDocument` and pagination;
- validates that a page request targets the opened book;
- accepts `LayoutRequest` with semantic anchor, typography and viewport;
- invokes the paginator;
- maps pagination output into the stable Reader `PageResult`;
- exposes visible lines plus first/last/next semantic anchors and progress.

It does not know anything about e-paper driver APIs.

## Page result

`PageResult` now carries the first concrete rendering payload used by the Reader layer:

- `lines[]`;
- first semantic position;
- last semantic position;
- optional previous anchor;
- optional next anchor;
- normalized progress.

Each line carries:

- UTF-8 text;
- semantic source position;
- x/y placement.

This remains a Reader/display-list representation rather than a framebuffer.

## Open document lifetime

The current `DocumentReaderEngine` receives references to:

- one already opened `BookDocument`;
- one `TextMeasurer`.

The caller owns their lifetime.

A later Reader session/document-provider layer may own loading/unloading documents from storage. That is intentionally not mixed into this MVP.

## Previous page

The MVP deliberately does **not** synthesize a previous-page anchor by subtracting characters.

`previous_anchor` remains empty until the Reader session/cache layer can provide deterministic history/checkpoint behavior.

This preserves the architectural rule that Previous must never be approximate.

## Failure behavior

The current interface returns no page when:

- the request targets another book;
- the semantic anchor cannot be paginated;
- the viewport/measurement path fails;
- the paginator reaches a non-success status.

The next implementation step can replace this coarse optional result with a structured Reader status/error without changing the pagination contract.

## Host test

The Reader Engine test covers:

- TXT parsing;
- first page layout;
- next-page request using the returned semantic anchor;
- line payload propagation;
- mismatched book rejection.

A fake fixed-width measurer is used only for deterministic host testing.

## Next step

The next Reader runtime layer should add a **ReaderSession** responsible for:

- current page;
- recent page-boundary history;
- deterministic Previous;
- small current/next/previous page cache;
- repagination invalidation after typography/orientation changes.

## Decisions fixed by this MVP

- Reader Engine consumes an already normalized/open document.
- Reader Engine bridges normalized content to Reader-level page results.
- Reader page payload is independent from framebuffer/e-paper APIs.
- Semantic anchors remain authoritative.
- Previous is left unset rather than approximated.
- Storage/document lifetime is not coupled into the paginator.
