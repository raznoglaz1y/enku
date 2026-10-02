# ENKU Reader Runtime Integration MVP

This document defines the first concrete integration between ReaderSession, AppState and RefreshService.

## Current event path

```text
PageNextRequested / PagePreviousRequested
→ ReaderRuntimeController
→ ReaderSession
→ AppState update
→ RefreshRequest(PageTurn)
→ RefreshService
```

This is the first implementation that connects reader navigation to application state and display-refresh scheduling.

## Preconditions

Page-turn events are handled only when:

- `AppState.screen == Reading`;
- ReaderSession is open.

Events outside Reading are ignored by this controller.

## Successful page turn

After ReaderSession returns a valid new page, the runtime updates:

- `current_book`;
- `reading_position`;
- `reading_progress`;
- `progress_dirty = true`.

Only after a valid ReaderSession transition does the application position advance.

## Refresh generation

Every successful visible page transition creates a new monotonic refresh generation.

Current request:

```text
RefreshClass::Full
RefreshReason::PageTurn
may_coalesce = false
may_defer = false
```

Here **Full** means the complete visible reading viewport changed. It does not pre-decide the physical e-paper waveform; the Refresh Manager/platform layer remains responsible for the actual panel update strategy.

## Refresh rejection

If RefreshService cannot accept the request:

- the new logical Reader/AppState is **not rolled back**;
- the runtime returns `RefreshRejected`;
- display recovery/retry can be handled by the refresh/display error path.

This follows the project rule that AppState remains authoritative even when physical refresh fails.

## Beginning / end

ReaderSession states map directly:

- `BeginningOfBook` → no state/refresh mutation;
- `EndOfBook` → no automatic wrap and no page refresh;
- `LayoutFailed` → no invalid reading-position commit.

Finished-book product handling remains a higher-level runtime concern.

## Persistence hand-off

The runtime does not synchronously persist on every page turn.

Instead it sets:

```text
progress_dirty = true
```

The persistence/checkpoint layer later commits according to the existing debounce/sleep/book-close policy.

## Current scope

Implemented:

- explicit PageNext/PagePrevious event handlers;
- ReaderSession invocation;
- AppState reading-position/progress synchronization;
- progress dirty marking;
- monotonic RefreshRequest generation;
- PageTurn refresh submission;
- non-Reading event ignore behavior;
- no logical rollback on refresh rejection.

Not yet implemented:

- OpenBookRequested integration;
- Back-to-Library checkpoint;
- end-of-book Finished transition;
- structured layout/display error events;
- typography/orientation event integration;
- render-plan payload/framebuffer generation.

## Decisions fixed by this MVP

- Page events do not directly mutate offsets.
- ReaderSession owns navigation semantics.
- AppState changes only after a valid ReaderSession page exists.
- Page turns mark progress dirty rather than synchronously persisting it.
- Visible page transitions generate RefreshRequest through one runtime boundary.
- Refresh rejection does not invalidate logical Reader state.
