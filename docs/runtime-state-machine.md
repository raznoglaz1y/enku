# ENKU Reader Runtime & State Machine

This document defines the baseline runtime behavior for opening books, navigating pages, changing reader settings, sleeping, waking and recovering from errors.

The goal is deterministic behavior: the same event in the same state should produce the same state transition and the same visible result.

## 1. Runtime principle

The Reader runtime is event-driven.

User input and system events update application state first. Rendering follows only when visible state changes.

Conceptually:

```text
Input/System Event
→ Event Queue
→ State Transition
→ Reader/Layout Work
→ Render Plan
→ Refresh Manager
→ Display
```

## 2. Top-level application states

ENKU uses a small set of product-level states:

- **Boot**
- **Library**
- **Book Opening**
- **Reading**
- **Reader Overlay**
- **Search**
- **Settings**
- **Import/Transfer**
- **Sleep**
- **Error/Recovery**

These are logical runtime states, not necessarily one-to-one UI screens.

## 3. Reader sub-state

When Reading is active, the runtime keeps:

- current `book_id`;
- current semantic reading position;
- current laid-out page;
- previous/next page anchors when available;
- effective typography;
- orientation;
- active bookmark state;
- visible reader chrome state;
- pending progress save state.

The displayed page is disposable. The semantic position is authoritative.

## 4. Opening a book

Canonical flow:

```text
Library
→ OpenBookRequested
→ Book Opening
→ loader validates Library/storage and opens ReaderSession
→ BookOpened
→ synchronize AppState from current Reader page
→ render first/current page
→ Reading
```

If book loading fails, `BookOpenFailed` closes any partial session and returns safely to Library with the requested book focused.

If no saved position exists, reading begins at the first valid content position.

If a saved anchor can no longer be resolved exactly, the Reader Engine should restore the nearest safe logical position.

## 5. Page Next

When the user requests Next:

1. ignore/reject duplicate input while the current page transition is not ready;
2. ask ReaderSession to advance using the current page's next semantic anchor;
3. reuse prefetched next page or ask Reader Engine to paginate it;
4. update current semantic position;
5. mark progress as dirty;
6. render the new page;
7. schedule persistence independently from display refresh.

The UI must not advance stored progress before a valid next page exists.

The first concrete implementation of this path is documented in [Reader Runtime Integration MVP](reader-runtime-mvp.md). It updates `AppState.reading_position` / `reading_progress`, marks `progress_dirty`, and emits a PageTurn `RefreshRequest` only after ReaderSession accepts the transition.

## 6. Page Previous

Previous returns to a deterministic prior logical page through ReaderSession.

The current MVP stores page-start semantic anchors actually visited during the session and keeps a disposable previous/current/next working set. If an adjacent page is not cached, it is rebuilt from the stored page-start anchor.

The runtime never approximates Previous by subtracting an arbitrary number of characters.

## 7. End of book

When no next readable content exists:

- do not wrap automatically to the beginning;
- expose the Finished state;
- allow the user to return to Library;
- preserve the final semantic position;
- mark the book Finished only through the defined completion behavior.

On EndOfBook, runtime marks the current book Finished, sets logical progress to 1.0, updates the Library summary and emits a visible status refresh. It does not wrap to the beginning.

Restarting a finished book changes its Library state back to Reading through an explicit product action.

## 8. Reader chrome / overlay

Normal Reading should remain content-first.

Opening Reader Menu or Quick Aa moves into a Reader Overlay state while preserving:

- book;
- semantic position;
- underlying page;
- orientation.

Closing the overlay returns to the exact underlying Reading state.

Opening/closing an overlay must not itself change reading progress.

## 9. Typography change

Canonical transition:

```text
Reading/Overlay
→ TypographyChanged
→ preserve semantic anchor
→ resolve effective settings
→ invalidate incompatible page cache
→ repaginate around anchor
→ render
→ return to Reading/Overlay
```

Manual typography changes switch the effective preset to Custom.

## 10. Orientation change

Canonical transition:

```text
Reading
→ OrientationChanged
→ preserve semantic anchor
→ update viewport
→ invalidate layout cache
→ repaginate
→ render full new layout
→ Reading
```

Pixel coordinates are not persisted.

## 11. Bookmark toggle

Bookmark creation stores:

- `book_id`;
- semantic position;
- optional section/chapter title;
- short text snippet;
- creation order/timestamp.

Toggling a bookmark updates state/persistence, but should only redraw a visible bookmark indicator if such an indicator is currently shown.

## 12. Table of contents navigation

Selecting a ToC entry:

1. resolve its semantic position;
2. preserve the current position as normal history/state;
3. paginate at the target;
4. update current reading position;
5. render;
6. return to Reading.

ToC navigation never depends on rendered page numbers.

## 13. Search flow

Canonical search flow:

```text
Reading
→ SearchOpen
→ query entry
→ normalized-text search
→ results list
→ result selected
→ resolve semantic position
→ paginate
→ highlight semantic match
→ Reading
```

The pre-search reading position should remain available so Back/Cancel can return the user to where search began.

Search highlighting is temporary UI state and does not alter book content.

## 14. Back behavior

Back is deterministic and context-sensitive.

When Back is pressed from Reading, runtime checkpoints dirty semantic position/progress before closing ReaderSession and returning to Library. If checkpointing fails, the Reader remains open so unsaved progress is not silently discarded.

Examples:

- Reading → Library;
- Reader Overlay → Reading;
- Search results → Search query/results context;
- Search canceled from reader → original Reading position;
- nested Settings → previous Settings level;
- recoverable error → previous safe state.

Back must never silently discard a destructive pending action without following the relevant confirmation rule.

## 15. Progress persistence

Progress saving should not block every page turn.

Recommended model:

- update in-memory semantic position immediately;
- mark progress dirty;
- persist after a short safe debounce, on important transitions, and before sleep/power-down where possible.

Important save points include:

- leaving the book;
- entering Sleep;
- explicit Power Off;
- application state checkpoint;
- selected periodic/debounced page progress.

The exact debounce interval remains an implementation detail.

## 16. Sleep

The detailed product/hardware split is defined in [Power, Sleep & Wake Model](power-model.md).

Canonical user-facing Sleep flow:

```text
active state
→ SleepRequested
→ finish/abort unsafe operation according to subsystem rules
→ persist dirty reader/app state
→ stop Wi-Fi / quiesce peripherals
→ prepare static sleep screen
→ final display update
→ e-paper sleep
→ enter verified system suspend mechanism
```

Panel-only sleep is not considered full ENKU Sleep.

No animation or live clock is maintained in Sleep.

Writes/imports/transfers may temporarily block automatic sleep.

## 17. Wake

Wake should restore the previous logical context when safe.

Typical flow:

```text
Sleep
→ Wake
→ restore application state
→ validate storage/book availability
→ restore current screen/book semantic position
→ repaginate if required
→ render
```

If the current book is unavailable, ENKU falls back to Library with a recoverable error notice rather than entering a broken Reading state.

## 18. Hard power-off / reboot recovery

Persistent state must be sufficient to recover without relying on the framebuffer.

After reboot:

- follow the staged startup sequence defined in [Boot & Startup Architecture](boot-model.md);
- validate settings/state version;
- recover or clean incomplete transactions;
- evaluate boot-loop / Recovery Mode before automatic restore;
- load Library index;
- restore last safe product state when appropriate;
- restore semantic reading position, not rendered page;
- regenerate any disposable layout/cache.

The exact hard power-off mechanism remains hardware-dependent.

## 19. Import/transfer interaction

Reading and Library state must remain logically independent from transfer progress.

During an active write/import:

- automatic sleep is suspended;
- destructive storage actions are serialized;
- display updates for progress are throttled;
- failed import does not alter existing valid books.

Manual reading may remain available only if storage access can be safely coordinated; this is an implementation choice to verify on hardware.

## 20. Error model

Errors are divided into:

- **recoverable** — user can return to the previous safe state;
- **book-specific** — one book cannot open/render;
- **storage-specific** — media unavailable or inconsistent;
- **system-level** — critical runtime failure.

Examples:

- corrupt book → Library remains available;
- missing current book after wake → return to Library;
- parser/layout failure → preserve last valid semantic position;
- failed import → existing Library remains unchanged.

Raw diagnostics belong in logs, not as primary user-facing copy.

The full error classification, logging and Recovery/Safe Mode behavior is defined in [Errors, Logging & Diagnostics](diagnostics-model.md).

## 21. Input locking and repeated actions

Physical controls can generate repeated or rapid events.

The runtime should:

- debounce hardware input;
- avoid overlapping page-layout operations;
- coalesce redundant navigation where safe;
- never start two display refreshes concurrently;
- prevent double activation of destructive actions.

Long-press behavior is defined at the input/action layer, not separately inside every screen.

## 22. State history

ENKU should keep only the navigation history needed for predictable Back behavior.

History is logical, not framebuffer-based.

It may include:

- previous screen/context;
- pre-search reading anchor;
- overlay parent;
- focused Library item;
- relevant scroll/focus state.

It should not grow without bound.

## 23. Persistence vs transient state

Persistent examples:

- current book;
- semantic reading position;
- Library view/filter/sort;
- reading settings;
- bookmarks;
- selected language;
- orientation;
- trusted Wi-Fi information.

Transient examples:

- active render plan;
- dirty rectangles;
- current display list;
- temporary search highlight;
- open animation-equivalent transition state;
- page cache.

Transient state can be rebuilt after reboot.

## 24. State transition table

| Current state | Event | Next state | Key action |
| --- | --- | --- | --- |
| Library | OpenBook | Book Opening | Resolve metadata/state and paginate |
| Book Opening | Success | Reading | Render first/current page |
| Book Opening | Failure | Error/Recovery | Return safely to Library |
| Reading | Next | Reading | Paginate next semantic position |
| Reading | Previous | Reading | Resolve deterministic previous page |
| Reading | OpenMenu | Reader Overlay | Preserve underlying page |
| Reader Overlay | Close | Reading | Restore underlying page |
| Reading | Search | Search | Preserve pre-search anchor |
| Search | OpenResult | Reading | Jump to semantic match |
| Reading | OrientationChanged | Reading | Repaginate around anchor |
| Reading | TypographyChanged | Reading | Repaginate around anchor |
| Any safe state | SleepRequested | Sleep | Persist state and sleep |
| Sleep | Wake | Previous safe state | Rebuild transient layout |
| Any | RecoverableError | Error/Recovery | Preserve valid state |
| Error/Recovery | Back/Recover | Previous safe state | Resume without corruption |

## 25. Decisions fixed by this document

- Reader runtime is event-driven.
- Semantic position is authoritative at every transition.
- Page turns update progress only after valid layout exists.
- Back behavior is deterministic and context-aware.
- Reader overlays do not alter reading position.
- Search preserves the pre-search reading position.
- Sleep persists state before entering low power where possible.
- Wake reconstructs transient layout rather than restoring framebuffer state.
- Progress writes are debounced/checkpointed rather than blocking every page turn.
- Display/layout work is serialized.
- Errors must return to a safe usable state without corrupting book/library data.


## Refresh back-pressure

The detailed refresh queue and e-paper update policy are defined in [Refresh Manager & E-Paper Update Policy](refresh-model.md).

Runtime rules:

- only one physical display update executes at a time;
- stale queued render work may be discarded when newer full-screen state supersedes it;
- page-turn input cannot create an unbounded refresh queue;
- display failure does not roll back authoritative AppState;
- hidden state changes do not produce display work.


## 26. Persisted safe runtime context

Successful transition into Reading persists `Reading + book_id` through `AppContextService`.

Back from Reading persists `Library` before destroying the active ReaderSession. If that context write fails, Back is aborted so the previously safe Reading state is not silently discarded.

At cold boot, `BootRestoreCoordinator` reuses the ordinary `OpenBookRequested` runtime path rather than creating a second special-purpose reader-opening implementation. Therefore automatic restore uses the same Library lookup, checkpoint restore, parser, ReaderSession and failure behavior as a user-initiated open.


## 27. Sleep and fast Wake

`SleepWakeCoordinator` owns the product-level Sleep transition without embedding hardware-specific PMU/deep-sleep logic in ReaderRuntime.

When Sleeping from Reading, the semantic position is checkpointed even if the normal page-turn debounce has not fired yet. This makes Sleep an explicit persistence boundary.

Fast Wake does not create a special reader-opening path. It calls the same persisted-context restore logic used by cold boot, which in turn uses `OpenBookRequested`, checkpoint restore and `ReaderBookLoader`. The difference is that Wake skips the full storage transaction scan when the suspended runtime still has mounted/valid storage.


## 28. Graceful Power Off

Power Off is a separate runtime lifecycle from Sleep.

`PowerOffCoordinator` treats shutdown as a persistence boundary: Reading position, Library summary and last-safe app context must commit before `PowerService::requestPowerOff()` is called. Network is disconnected only after those writes succeed.

The next boot restores through the normal cold-boot path, so a powered-off device does not depend on retained RAM or framebuffer state.
