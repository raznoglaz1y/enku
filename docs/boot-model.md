# ENKU Boot & Startup Architecture

This document defines the Reader v1 boot sequence, startup checkpoints, restore rules and Recovery Mode entry points.

The goal is a deterministic, fast and failure-tolerant startup path that never depends on stale framebuffer state or blindly reopens a failing book.

## 1. Core principle

Boot is a staged state machine.

Each stage performs one bounded responsibility, reports structured diagnostics, and advances only after its required invariants are satisfied.

Conceptually:

```text
Reset / power-on
→ capture reset/wake reason
→ establish boot diagnostic marker
→ initialize minimum platform services
→ validate persistent state
→ choose Normal Boot or Recovery Mode
→ mount/validate storage
→ load settings + Library
→ initialize display/input/power services
→ restore logical product state
→ render first usable screen
→ mark boot stable
```

## 2. Startup priorities

The boot path prioritizes:

1. data safety;
2. recoverability;
3. first usable screen;
4. optional/background services.

Wi-Fi, Library rescans and other nonessential work must not delay the first usable local screen unless they are required for the current state.

## 3. Early boot phase

Early boot performs only operations needed to understand why the device started and whether normal startup is safe.

Tasks:

- initialize minimal logging/diagnostics;
- capture platform reset reason;
- capture wake reason where available;
- read boot-loop/recovery marker;
- mark this boot as in-progress;
- identify firmware/build/schema versions;
- initialize only platform primitives required by later stages.

Do not:

- open the previous book;
- start Wi-Fi;
- begin Library rescan;
- render complex UI;
- mutate user book files.

## 4. Boot-loop detection

Boot-loop detection is evaluated before restoring the previous user context.

Conceptually:

```text
boot begins
→ increment/record incomplete-boot marker
→ if repeated incomplete boots exceed threshold:
     request Recovery Mode
→ after first stable usable screen:
     clear incomplete-boot condition
```

The exact threshold remains an implementation constant.

Recovery Mode must not repeatedly attempt the same failing automatic restore path.

## 5. Minimum platform initialization

The next phase initializes only services needed to reach a safe screen.

Likely order:

1. power/PMU status;
2. storage transport/filesystem;
3. persistence backend;
4. display driver;
5. input controls;
6. clock/RTC if required by status UI.

The exact low-level ordering may change after real-board bring-up if vendor dependencies require it.

## 6. Persistence validation

Before restoring product state:

- load latest valid global settings generation;
- validate schema version;
- load critical diagnostics/recovery record;
- recover latest valid Library generation;
- validate per-book state only when needed;
- inspect incomplete transactional operations.

Invalid records use the A/B recovery rules from the persistence model.

A failed settings record should not prevent Library access if safe defaults can be used.

## 7. Temporary transaction recovery

Startup inspects the transaction staging area.

Possible outcomes:

- clearly incomplete/uncommitted import → clean temporary data;
- valid committed Library entry with stale temporary artifact → remove stale temporary artifact;
- ambiguous transaction → preserve source/user data and surface recoverable diagnostic.

Boot must never delete original book files merely to make startup succeed.

## 8. Storage unavailable

If expected storage is unavailable:

- do not crash;
- do not replace the Library with an empty index;
- do not persist "no books" over a previously valid Library state;
- show the product's storage-unavailable flow;
- allow retry/reinsert behavior.

If core settings are stored separately from removable media, those settings may still load normally.

## 9. Display initialization

Display initialization happens after enough state is known to decide what should be shown.

A minimal boot screen is optional.

ENKU should avoid multiple unnecessary full refreshes during startup.

Preferred behavior:

- initialize panel;
- compose the first meaningful usable screen;
- issue the minimum required refresh sequence;
- defer cosmetic/background changes.

If display initialization fails, escalate through diagnostics/Recovery Mode rather than continuing blind.

## 10. Input initialization

Physical inputs should be available by the time the first interactive screen is shown.

Input events arriving before the application is ready may be ignored or buffered in a strictly bounded way.

Boot must not accidentally interpret a held button as repeated navigation through restored screens.

## 11. Normal restore decision

Normal startup restores logical context, not raw visual state.

Restore candidates:

- Library with previous focus/filter/sort;
- Reading at the saved semantic position;
- Settings/root context where appropriate.

ENKU should not automatically restore transient overlays, dialogs, search highlights or incomplete keyboard entry.

## 12. Restoring Reading

Reading may be restored only after:

- Library entry exists;
- source book is available;
- per-book state is valid/recoverable;
- parser can open the book;
- saved semantic position resolves safely.

Then:

```text
load book
→ resolve semantic anchor
→ resolve typography/orientation
→ paginate
→ render
→ Reading
```

If any required step fails, return to Library with a recoverable error rather than repeatedly retrying the same book.

## 13. Cold boot vs wake restore

The product distinguishes:

- cold boot / PMU power-on;
- software reboot;
- wake from Suspended where the platform can identify it.

Wake may use a faster restore path if platform state is trustworthy.

However, all paths must be able to reconstruct from persistent logical state.

No startup path depends on retained framebuffer contents.

## 14. First-start flow

If no valid initial settings exist because the device is genuinely unconfigured:

```text
Boot
→ First Start / Language
→ Orientation
→ Controls
→ Library
```

Corrupt settings must not automatically be mistaken for a brand-new device if recoverable evidence says the device was previously configured.

## 15. Recovery Mode entry

Recovery Mode may be entered when:

- boot-loop threshold is exceeded;
- persistence core cannot be recovered;
- storage initialization repeatedly fails in a way that blocks normal startup;
- display/system initialization reaches a critical failure path;
- explicit service/recovery action requests it.

Recovery Mode avoids:

- auto-opening last book;
- automatic Wi-Fi startup;
- nonessential cache loading;
- background scans until requested.

## 16. First usable screen checkpoint

Boot is considered product-stable only after:

- core state is valid;
- display has successfully rendered an interactive screen;
- input is active;
- no critical startup transaction remains unresolved.

At that point ENKU marks the boot as stable and clears the incomplete-boot marker.

This checkpoint is important for boot-loop detection.

## 17. Background startup work

After the first usable screen, ENKU may perform low-priority work such as:

- cover-cache validation;
- optional Library reconciliation;
- battery/status refinement;
- trusted-network connection when product policy requires it;
- cleanup of noncritical stale cache files.

Background work must yield to page turns and direct user input.

## 18. Wi-Fi startup policy

Wi-Fi is not part of the critical boot path.

Default behavior:

- do not start Wi-Fi before the first usable local screen;
- do not delay book reading for network connection;
- connect later only according to saved product policy or an explicit user action.

Recovery Mode keeps Wi-Fi off unless explicitly requested.

## 19. Startup timing instrumentation

Development builds should record stage timings.

Suggested checkpoints:

- reset → diagnostics ready;
- storage init;
- persistence load;
- display init;
- Library load;
- book restore/pagination;
- first usable screen;
- boot stable.

This allows startup optimization to be based on measurements rather than assumptions.

## 20. Boot event model

Core runtime may use explicit events such as:

```text
BootStarted
PlatformReady
StorageReady
PersistenceReady
RecoveryRequired
RestoreRequested
FirstScreenRendered
BootStable
```

The exact reducer implementation remains open, but boot must remain staged and observable.

## 21. Data-write policy during boot

Boot performs the minimum writes necessary for:

- boot-loop marker;
- migration/recovery commit when required;
- cleanup of clearly disposable temporary transactions.

Do not rewrite settings, Library or per-book state simply because they were loaded.

## 22. Firmware/schema compatibility

Startup validates:

- firmware-supported persistence schema;
- Library schema;
- settings schema;
- per-book state schema when opened.

If migration is supported:

```text
load old valid generation
→ migrate in memory
→ write new generation
→ validate
→ continue
```

A newer unsupported schema must produce a controlled recovery/error state rather than silent downgrade corruption.

## 23. Startup failure isolation

Failures should be isolated to the smallest subsystem possible.

Examples:

- corrupt cover cache → ignore/regenerate later;
- one corrupt book state → Library still opens;
- failed auto-open book → Library;
- Wi-Fi failure → local reader unaffected;
- settings failure → safe defaults if possible;
- one invalid Library generation → use the other;
- total Library index failure → Recovery/Rescan path without deleting books.

## 24. Decisions fixed by this document

- Boot is a staged state machine.
- Boot-loop detection happens before automatic context restoration.
- Wi-Fi is not part of the critical startup path.
- First usable local screen is prioritized over background work.
- Storage absence never overwrites an existing Library as empty.
- Reading restore requires successful validation of the book and semantic anchor.
- Transient overlays/search/input state are not automatically restored.
- Recovery Mode suppresses automatic reopening and optional services.
- Boot becomes stable only after an interactive screen renders successfully.
- Startup timing is instrumented in development builds.
- Boot does not rewrite valid persistence records merely by loading them.


## 25. Storage startup recovery MVP

The first concrete storage/persistence startup coordinator is now implemented as `StorageStartupCoordinator`.

Current bounded boot flow:

```text
BootStage::Storage
→ BootStage::Persistence
→ CborLibraryService::load()
→ BootStage::RecoveryDecision
→ inspect /system/tmp
→ clean only clearly stale committed artifacts
→ preserve ambiguous/incomplete uploads
→ Library or ErrorRecovery
```

Recovery decisions:

- valid Library + no pending tmp → normal Library startup;
- tmp content already indexed by fingerprint → stale artifact, safe to remove;
- new/invalid/unsupported tmp content → preserve file and enter Recovery Mode;
- unrecoverable Library index → enter Recovery Mode rather than treating it as an empty Library;
- tmp cleanup/storage I/O failure → enter Recovery Mode.

The coordinator intentionally does not auto-import ambiguous tmp files after reboot. User/source data is preserved until a later recovery action explicitly retries or removes it.

This MVP covers the storage/persistence part of boot only. Display, input, settings, boot-loop markers and automatic reading restore remain separate boot stages.


## 26. Last-safe context and Reading restore MVP

ENKU now persists a small independent application restore record:

```text
/system/context.a.cbor
/system/context.b.cbor
```

The record intentionally stores only a safe top-level context:

- `Library`; or
- `Reading + book_id`.

Transient overlays, search state, render state and page-cache data are never stored in this record.

`ReaderRuntimeController` updates it at safe transitions:

- successful book open → `Reading + book_id`;
- successful Back-to-Library transition → `Library`.

`BootRestoreCoordinator` runs after storage recovery:

```text
StorageStartupCoordinator
→ load app context
→ Library context: show Library
→ Reading context: validate Library entry
→ ReaderRuntimeController(OpenBookRequested)
→ ReaderCheckpointService restore
→ ReaderBookLoader
→ Reading
```

If automatic restore fails because the source disappeared, the checkpoint is invalid, parsing/layout fails or the book cannot be opened, boot falls back to Library, focuses the affected book and rewrites the safe context to `Library`. This prevents repeated automatic reopen loops on subsequent boots.

If the app-context record itself is missing, ENKU initializes a safe Library context. If it is corrupt but writable, ENKU resets it to Library rather than treating a context-only failure as loss of the Library.

The first host integration test models a cold reboot with new service/runtime objects and verifies that both the book and the persisted semantic offset are restored.


## 27. Fast Wake vs cold boot

`BootRestoreCoordinator` now exposes two entry paths:

- `run()` — cold boot: perform `StorageStartupCoordinator` recovery first, then restore safe app context;
- `restoreContextOnly()` — fast Wake from logical Suspended state: reuse current mounted storage/Library and restore only the persisted safe context.

Both paths converge on the same ReaderRuntime open/checkpoint logic and the same fallback-to-Library behavior. This avoids maintaining separate cold-boot and wake-specific reader restoration implementations.


## 28. Persistent boot-loop protection MVP

Cold boot now uses `CborBootLoopService` with an A/B marker:

```text
/system/boot-marker.a.cbor
/system/boot-marker.b.cbor
```

The marker stores:

- consecutive incomplete cold-boot count;
- whether the previous boot reached the stable checkpoint.

Cold-boot sequence:

```text
storage recovery
→ beginBoot()
→ increment incomplete count
→ if count >= 3: Recovery Mode
→ otherwise restore safe app context
→ first stable Library/Reading state
→ markStable()
→ reset count to 0
```

The threshold is currently fixed at three consecutive incomplete cold boots.

The gate is evaluated before automatic Reading restore, so a repeatedly failing last book is not reopened indefinitely.

Fast Wake from logical Suspended state does not increment or clear the cold-boot marker. It continues to use `restoreContextOnly()`.

Recovery/service code may explicitly call `markStable()` after the underlying cause has been handled, allowing the next cold boot to start a fresh attempt sequence.

Host integration tests cover two incomplete boots followed by a third boot entering Recovery, then explicit marker recovery followed by a normal stable Library boot.
