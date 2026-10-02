# ENKU Reader Session MVP

This document defines the first stateful Reader navigation layer above `ReaderEngine`.

## Current path

```text
BookDocument
→ DocumentReaderEngine
→ ReaderSession
→ previous / current / next page cache
→ runtime/UI
```

## Responsibilities

`ReaderSession` owns short-lived reading-navigation state:

- current page;
- previous adjacent page cache;
- next adjacent page cache;
- deterministic history of page-start semantic anchors;
- current typography and viewport request;
- layout invalidation after typography/orientation changes.

It does not own persistence, book-file loading or e-paper refresh.

## Deterministic Previous

Previous-page navigation is now based on page-start anchors that were actually visited.

On successful Next:

1. current page start is pushed into history;
2. next page becomes current;
3. the previous page remains cached.

On Previous:

1. the last visited page-start anchor is popped;
2. cached previous page is reused when available;
3. otherwise the Reader Engine lays out exactly from that stored anchor.

ENKU does not subtract an arbitrary character count.

## Three-page working set

The MVP keeps a small disposable working set:

- previous page;
- current page;
- next page.

Next is prefetched after a successful current-page layout.

Older Previous pages are reconstructed from deterministic history when needed.

The cache is an optimization only. Semantic anchors remain authoritative.

## Beginning/end behavior

If no history entry exists:

```text
Previous → BeginningOfBook
```

If the current page has no `next_anchor`:

```text
Next → EndOfBook
```

The session does not wrap around automatically.

## Layout invalidation

Typography or viewport/orientation changes invalidate adjacent page caches.

The session:

1. preserves the current page's semantic start anchor;
2. applies new typography/viewport;
3. clears page-boundary history because old boundaries are no longer valid;
4. repaginates around the semantic anchor;
5. prefetches the new next page.

This intentionally prioritizes stable logical position over preserving old rendered-page history.

## Current limitations

The MVP does not yet provide:

- persistence of navigation history across reboot;
- page checkpoints beyond the current session;
- reverse reconstruction before the first visited anchor;
- bounded long-session history compaction;
- asynchronous/background prefetch;
- render/display back-pressure integration.

These can be added without changing the authoritative semantic-position model.

## Decisions fixed by this MVP

- Runtime page navigation is stateful above the stateless Reader Engine.
- Previous uses real visited page-start anchors.
- Current/previous/next cache is disposable.
- Cache failure does not change logical reading identity.
- Typography/orientation invalidates old page boundaries.
- Page history is not persisted as the reading position.
