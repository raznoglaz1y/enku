# ENKU Errors, Logging & Diagnostics

This document defines how ENKU separates user-facing errors, developer diagnostics, runtime logs and recovery information.

The goal is to make failures understandable to the reader while still preserving enough technical context to debug problems without a live debugger.

## 1. Core principle

ENKU distinguishes three layers:

```text
Internal diagnostic
→ Product error classification
→ Localized user-facing message
```

Raw parser, filesystem, driver or exception text must not be shown directly as normal UI copy.

## 2. Error domains

Every structured error belongs to one domain.

Initial domains:

- Application
- Reader
- Parser
- Layout
- Storage
- Persistence
- Import
- Network
- Display
- Input
- Power
- Localization
- System

The domain helps diagnostics and recovery code without exposing subsystem jargon to the user.

## 3. Error severity

Initial severity levels:

- **Info** — noteworthy state, no failure;
- **Warning** — degraded/non-critical condition;
- **RecoverableError** — operation failed, product can return to a safe state;
- **CriticalError** — current subsystem/runtime cannot safely continue normally;
- **FatalError** — firmware must enter recovery/reboot/fail-safe path.

Severity is independent from log verbosity.

A parser error can be recoverable. A storage corruption event may be critical. A debug log line is not automatically an error.

## 4. Stable error codes

Errors use stable machine-readable codes.

Conceptually:

```text
storage.card_missing
storage.read_failed
persistence.invalid_record
parser.unsupported_format
parser.corrupt_file
reader.position_unresolvable
display.refresh_failed
network.connection_failed
power.suspend_failed
system.out_of_memory
```

Code names remain stable even when UI wording changes.

## 5. Structured error object

A structured error should contain:

- domain;
- severity;
- stable code;
- optional operation/context identifier;
- optional related `book_id`;
- optional lower-level cause code;
- optional numeric/native error value;
- source location in development builds where practical;
- user-message localization key;
- recovery hint/action;
- timestamp/monotonic sequence if available.

Large raw strings should not be required to pass errors through application state.

## 6. User-facing errors

User-facing messages are localized through the normal localization layer.

A product error presentation should answer only what the reader needs:

- what failed;
- whether their book/data is safe;
- what action is available now.

Examples:

```text
"This book could not be opened."
"The memory card is not available."
"Import was interrupted. The original book was not changed."
"Not enough free space to add this book."
```

Technical details belong in diagnostics.

## 7. Recovery actions

Structured errors may advertise one or more product-level recovery actions:

- Retry
- Back
- ReturnToLibrary
- ReinsertStorage
- Reconnect
- RemoveTemporaryData
- ResetAffectedState
- Reboot
- PowerOff

The UI decides which actions are currently valid and which one receives safe default focus.

Destructive recovery must never be the default focus.

## 8. Recoverable failure behavior

A recoverable error should preserve the last valid state.

Examples:

- corrupt EPUB → return to Library, keep other books usable;
- failed page layout → preserve last valid semantic position;
- interrupted import → existing Library remains unchanged;
- cover decode failure → use fallback cover;
- missing translation → fall back to English;
- one corrupt per-book state record → Library remains available.

## 9. Critical and fatal failure behavior

Critical/fatal conditions must avoid endless reboot loops and repeated destructive writes.

Potential triggers:

- repeated boot failure during state restoration;
- unrecoverable persistence initialization;
- display initialization failure;
- severe memory/resource failure;
- internal invariant violation.

The exact hardware reset mechanism is platform-specific, but the product flow is fixed:

```text
detect critical/fatal condition
→ record compact diagnostic if safe
→ stop unsafe writes
→ attempt safe product recovery
→ if recovery fails, enter Recovery/Safe Mode
```

## 10. Safe Mode / Recovery Mode

ENKU should have a minimal recovery path that does not depend on the full Library/Reader stack.

Recovery Mode goals:

- boot with conservative settings;
- avoid automatically reopening the previous book;
- avoid starting Wi-Fi unless explicitly requested;
- skip disposable caches;
- allow Library rescan/recovery where possible;
- expose basic device/firmware diagnostic information;
- allow reboot or power-off.

Recovery Mode must not automatically erase user books.

The detailed startup sequence and the exact point where boot becomes stable are defined in [Boot & Startup Architecture](boot-model.md).

## 11. Boot-loop detection

ENKU should track a compact boot/recovery marker.

Conceptually:

- boot starts → mark boot-in-progress;
- core initialization succeeds → mark boot-stable;
- repeated boots before stable marker indicate a possible boot loop.

After a small threshold of consecutive failed starts, ENKU should enter Recovery Mode instead of repeatedly attempting the same failing restore path.

The exact threshold remains an implementation constant to validate during firmware testing.

## 12. Logging levels

Runtime log levels:

- Trace
- Debug
- Info
- Warning
- Error
- Critical

Recommended release behavior:

- Trace disabled;
- Debug normally disabled;
- Info retained selectively;
- Warning/Error/Critical retained for diagnostics.

Development builds may enable more verbose output.

## 13. Log categories

Logs should also carry a subsystem/category.

Examples:

```text
core
input
ui
reader
parser.epub
parser.fb2
parser.txt
layout
storage
persistence
import
wifi
display
power
localization
web
```

This makes filtering possible without parsing message text.

## 14. Structured logging

Logging should prefer stable event fields over ad-hoc prose.

Conceptually:

```text
level=Warning
category=parser.epub
event=resource_skipped
book_id=<id>
code=unsupported_media_type
```

Human-readable messages may accompany the event in development logs.

## 15. Log ring buffer

ENKU should maintain a bounded in-memory diagnostic ring buffer.

Goals:

- recent history survives long enough to inspect after a failure;
- logging cannot consume unbounded RAM;
- repetitive messages can be coalesced/rate-limited;
- normal reading does not constantly write diagnostics to flash/SD.

Exact ring size is chosen after memory profiling.

## 16. Persistent diagnostics

ENKU must not persist every log line.

Persistent diagnostics are reserved for useful failure summaries such as:

- last critical error;
- reboot/recovery reason;
- failed boot counter;
- firmware version/build id;
- persistence recovery event;
- storage mount failure;
- last fatal subsystem/code.

A small versioned diagnostic record can use the same crash-tolerant persistence principles as other ENKU state.

## 17. Flash/SD wear policy

Normal logs are not continuously appended to flash or SD.

Persistent writes should happen only for:

- critical/fatal summaries;
- explicit diagnostic export;
- selected recovery markers;
- developer mode when deliberately enabled.

This prevents diagnostics from becoming a hidden write-amplification source.

## 18. Diagnostic export

A future local web-management/device diagnostic function should be able to export a support bundle.

Potential contents:

- firmware version/build id;
- board/hardware profile;
- recent structured log ring;
- last error/recovery records;
- storage status;
- persistence schema versions;
- memory statistics;
- display/power capability flags.

It must not automatically include book text, Wi-Fi passwords or other unnecessary private content.

## 19. Privacy / sensitive data

Logs must avoid secrets and reading content by default.

Do not log:

- Wi-Fi passwords;
- full book text;
- arbitrary search queries unless explicitly needed in a development build;
- raw uploaded file contents;
- private web-session tokens.

Book identifiers/fingerprints may be shortened/redacted in user-exportable diagnostics where appropriate.

## 20. Assertions and invariants

Development builds may use assertions aggressively for programmer errors.

Release builds should convert recoverable invariant failures into:

- structured diagnostic;
- safe subsystem reset/recovery where possible;
- Recovery Mode for repeated/unrecoverable cases.

An assertion must not be the only strategy for ordinary malformed user files.

## 21. Out-of-memory behavior

Allocation failure must be treated as a first-class error on ESP32-S3.

Subsystems should:

- return explicit failure rather than dereference null;
- release disposable caches where useful;
- preserve current semantic position;
- avoid starting another large operation;
- show a product-level failure if the operation cannot continue.

Repeated/system-wide allocation failure may escalate to Recovery Mode/reboot.

## 22. Watchdog/reboot diagnostics

If the chosen framework exposes reset/wakeup reason, ENKU should capture it early during boot.

Useful reasons may include:

- cold boot;
- software restart;
- watchdog/reset;
- brownout;
- wake from suspend;
- PMU/cold power-on where detectable.

The exact platform enum remains implementation-specific.

## 23. Error-to-UI mapping

Error codes map to localization keys centrally.

Example:

```text
parser.corrupt_file
→ error.book_open.title
→ error.book_open.body
→ actions: Back / ReturnToLibrary
```

Screens do not invent error wording per call site.

## 24. Development diagnostics

Development firmware should be able to expose:

- heap/PSRAM usage;
- task/stack information if the selected framework supports it;
- active screen/state;
- current book id;
- semantic reading anchor;
- display refresh counters;
- partial/full refresh counts;
- persistence generation/schema;
- storage free space;
- Wi-Fi state;
- wake/reset reason.

These are diagnostic facts, not normal product UI.

## 25. Version/build identity

Every diagnostic report should identify firmware uniquely enough to reproduce issues.

At minimum:

- semantic firmware version when releases begin;
- build identifier/commit when available;
- persistence schema version;
- target board profile.

## 26. Decisions fixed by this document

- Product errors and raw diagnostics are separate.
- Errors have stable domain/severity/code fields.
- User-facing errors are localized and action-oriented.
- Recoverable failures preserve the last valid product state.
- ENKU has a minimal Recovery/Safe Mode concept.
- Boot-loop detection prevents repeating the same failing restore indefinitely.
- Runtime logs are structured, categorized and bounded.
- Normal logs are not continuously written to persistent storage.
- Persistent diagnostics store compact failure summaries only.
- Diagnostic export must avoid passwords and book content by default.
- Allocation failure is handled explicitly.
- Error-to-UI mapping is centralized.
