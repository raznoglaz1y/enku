# ENKU Book Loader / Document Provider MVP

This document defines the first concrete service that turns a Library record plus source bytes into an opened ReaderSession.

## Current path

```text
LibraryService
→ BookSourceService
→ ReaderBookLoader
→ format parser
→ BookDocument
→ DocumentReaderEngine
→ ReaderSession
```

This removes the need for runtime code to know parser or document-lifetime details.

## Responsibilities

`ReaderBookLoader` owns the lifetime of:

- the currently normalized `BookDocument`;
- its `DocumentReaderEngine`;
- the active `ReaderSession`.

The caller receives a stable ReaderSession pointer only while the loader remains open.

## Source abstraction

Raw book bytes are provided through `BookSourceService`.

The first interface is deliberately small:

```text
readSource(BookRecord, bytes)
```

This is suitable for host tests and first integration.

It is **not** a commitment that production EPUB/FB2 books will always be loaded completely into RAM. Streaming/ranged source access can replace this MVP interface after filesystem and memory profiling.

## Format dispatch

Current implementation:

- TXT → supported through the existing `TxtParser`;
- EPUB → returns `UnsupportedFormat`;
- FB2 → returns `UnsupportedFormat`.

This is intentional until those parsers exist.

## Restore anchor

The open request may carry a saved semantic position.

If it belongs to the requested `book_id`, it is used as the ReaderSession anchor.

Otherwise the loader starts from:

```text
book_id + empty section id + offset 0
```

The Reader Engine resolves the first document section.

## Failure isolation

The loader returns structured status:

- NotFound;
- SourceUnavailable;
- SourceReadFailed;
- UnsupportedFormat;
- ParseFailed;
- SessionOpenFailed.

On every failed open, partially created document/engine/session state is cleared.

## Lifetime order

Close/reset order is deliberate:

```text
ReaderSession
→ DocumentReaderEngine
→ BookDocument
```

This prevents the engine/session from retaining references to a destroyed document.

## Current limitations

Not yet implemented:

- EPUB parser;
- FB2 parser;
- streaming/ranged source reading;
- asynchronous loading;
- loader progress events;
- structured parser error details;
- asynchronous loading / progress reporting.

The Reader runtime now invokes this loader directly for `OpenBookRequested` and completes through the existing `BookOpened` / `BookOpenFailed` lifecycle paths. No external/manual loader event is required for the synchronous MVP.

## Decisions fixed by this MVP

- Runtime/UI do not own parser/document lifetimes.
- A single loader owns one active normalized document/session.
- Format selection happens behind the loader boundary.
- Failed opens clear partial state.
- Semantic saved positions are accepted only for the same book.
- Full-file byte loading is an MVP implementation detail, not a production requirement.
