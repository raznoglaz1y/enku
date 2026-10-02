# ENKU Reader Runtime Integration MVP

This document defines the first concrete integration between ReaderSession, AppState and RefreshService.

## Current event paths

```text
OpenBookRequested
→ Book Opening
→ ReaderBookLoader opens the source/document/session
→ BookOpened / BookOpenFailed completion path
→ Reading / Library

PageNextRequested / PagePreviousRequested
→ ReaderRuntimeController
→ ReaderSession
→ AppState update
→ RefreshRequest(PageTurn)
→ RefreshService

BackRequested
→ checkpoint dirty reader state
→ update Library summary
→ close ReaderSession
→ Library

EndOfBook
→ mark Finished
→ Library summary = Finished / 100%
→ status refresh
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

ReaderSession states map into product behavior:

- `BeginningOfBook` → no state/refresh mutation;
- `EndOfBook` → mark current book Finished, progress = 1.0, update Library summary and request a status refresh;
- `LayoutFailed` → no invalid reading-position commit.

The Reader never wraps automatically from the end to the beginning.

## Persistence hand-off

The runtime does not synchronously persist on every page turn.

Instead it sets:

```text
progress_dirty = true
```

The persistence/checkpoint layer later commits according to the existing debounce/sleep/book-close policy.

## Book open lifecycle

`OpenBookRequested` now moves the app from Library to Book Opening and records the requested `book_id`.

Actual storage/parser/document construction is implemented by the first [Book Loader / Document Provider MVP](book-loader-mvp.md). `ReaderRuntimeController` now invokes that loader directly when it receives `OpenBookRequested`.

The completion semantics remain expressed through the existing lifecycle handlers:

- successful load → `BookOpened` path validates the loader-owned session, switches to Reading and synchronizes the visible page into AppState;
- failed load → `BookOpenFailed` path clears loader state and returns safely to Library.

The current implementation is synchronous, but the success/failure lifecycle remains explicit so loading can later become asynchronous without changing screen/state semantics.

## Back / checkpoint

Back from Reading:

1. checkpoints dirty semantic position/progress through `ReaderCheckpointService`;
2. updates the Library summary;
3. closes ReaderSession;
4. restores Library focus to the book just closed;
5. clears transient current-book state;
6. requests a screen-change refresh.

Checkpoint failure leaves the Reader open rather than silently discarding unsaved progress.

## Current scope

Implemented:

- OpenBookRequested → Book Opening → automatic ReaderBookLoader invocation;
- automatic success/failure completion through BookOpened / BookOpenFailed lifecycle handlers;
- explicit PageNext/PagePrevious handlers;
- ReaderSession invocation;
- AppState reading-position/progress synchronization;
- progress dirty marking;
- monotonic RefreshRequest generation;
- PageTurn refresh submission;
- EndOfBook → Finished summary transition;
- Back-to-Library checkpoint and session close;
- non-Reading page-event ignore behavior;
- no logical rollback on refresh rejection.

Not yet implemented:

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
