# ENKU Persistence Backend

This document fixes the Reader v1 persistence strategy for Library metadata, settings, reading progress, bookmarks and transactional recovery.

The design goal is to keep writes small, recovery deterministic and the on-disk format understandable without bringing a full relational database into the first firmware revision.

## 1. Decision

Reader v1 will use **versioned CBOR records/files with explicit integrity metadata and generation-based recovery**.

ENKU will **not use SQLite for Reader v1**.

JSON may be used for debugging, export or development tools, but it is not the primary runtime persistence format.

## 2. Why CBOR

CBOR fits the ENKU constraints well:

- compact binary representation;
- schema can evolve through explicit version fields;
- strings and structured metadata remain straightforward;
- lower storage and parsing overhead than JSON;
- no relational database engine is required;
- records can be read/written independently;
- suitable libraries exist for embedded C/C++ ecosystems.

The exact CBOR library is intentionally not selected until the firmware framework is finalized.

## 3. Why not SQLite in v1

SQLite is technically possible on ESP32-class hardware, but ENKU does not currently need relational queries or a general SQL engine.

For Reader v1 it would add:

- a larger dependency surface;
- more filesystem/database integration work;
- page/cache tuning;
- additional write/recovery behavior to validate;
- complexity that does not directly improve reading.

SQLite can be reconsidered later if the Library model grows enough to justify it.

## 4. Why not plain JSON

JSON is useful for humans but is not ideal as the primary runtime format because it is:

- larger on disk;
- slower/more allocation-heavy to parse in constrained environments;
- inefficient for frequently loaded structured records;
- easy to accidentally rewrite as one large document.

Human-readable export remains a useful future maintenance/debug feature.

## 5. Physical layout

Reader v1 persistence should be separated by write frequency and failure domain.

Proposed layout:

```text
/books/
  <original book files>

/system/
  library.a.cbor
  library.b.cbor

  settings.a.cbor
  settings.b.cbor

/system/state/
  <book_id>.a.cbor
  <book_id>.b.cbor

/system/covers/
  <book_id>.<cache-format>

/system/tmp/
  <staged import/replace data>
```

The A/B pair is a generation pair, not two independent copies to update simultaneously.

## 6. Record envelope

Every persisted CBOR record should contain a small common envelope.

Conceptually:

```text
schema_version
record_type
generation
payload
integrity
```

The exact binary representation of integrity metadata is implementation-defined, but the reader must be able to detect an invalid/incomplete record.

Recommended integrity fields:

- payload length where useful;
- CRC/checksum;
- generation counter.

## 7. Generation-based recovery

For each A/B pair:

1. read both records;
2. validate structure/schema/integrity;
3. discard invalid records;
4. choose the valid record with the highest generation;
5. if only one record is valid, use it;
6. if neither record is valid, enter subsystem-specific recovery.

This avoids depending entirely on rename being perfectly crash-atomic on every filesystem/media combination.

## 8. Commit sequence

For a normal update:

1. identify the inactive/older slot;
2. serialize the complete new record;
3. write it to that slot;
4. flush/close the file through the storage abstraction;
5. re-open or validate if required;
6. only then consider the new generation committed;
7. leave the previous valid generation untouched until the new one is known-good.

A failed write therefore leaves the older generation recoverable.

## 9. Library index

The Library index changes relatively infrequently.

It contains:

- stable book identity;
- normalized metadata;
- source path/fingerprint;
- cover reference;
- Library reading state;
- summary progress used by Library UI;
- timestamps/order data;
- import/schema migration metadata.

It should **not** be rewritten on every page turn.

## 10. Per-book state

Frequently changing state lives separately under `/system/state/`.

A per-book state record contains:

- `book_id`;
- semantic reading position;
- normalized progress;
- reading state;
- bookmarks;
- per-book reading overrides;
- last-opened information;
- state schema version.

Separating it from the Library index reduces write amplification during normal reading.

## 11. Progress checkpointing

Page turns update progress in RAM immediately.

Persistent progress writes are checkpointed according to the runtime policy, for example:

- after a debounce interval;
- when leaving a book;
- before Sleep;
- before graceful Power Off;
- at selected safe state transitions.

A page turn must not synchronously rewrite the entire Library database.

## 12. Global settings

Global settings use their own A/B CBOR record.

Examples:

- UI language;
- orientation;
- Library view/filter/sort;
- reading defaults;
- sleep timeout;
- Wi-Fi product preferences that are appropriate for this storage layer.

Sensitive Wi-Fi credentials may be stored using the platform's secure/nonvolatile mechanism instead of ordinary CBOR files if the selected framework/platform supports a better option.

## 13. Bookmarks

Bookmarks live in the per-book state record for Reader v1.

This keeps all book-specific logical state together and makes delete/recovery straightforward.

If profiling later shows that very large bookmark collections justify independent storage, the schema can evolve.

## 14. Cover cache

Cover cache files are disposable derived assets.

They do not require A/B persistence.

If a cover cache is missing or corrupt:

- invalidate it;
- regenerate from source/replacement metadata when possible;
- otherwise use the deterministic fallback cover.

The Library must remain valid even if every cover cache file is deleted.

## 15. Transactional imports

Book-file import remains separate from metadata persistence.

Canonical high-level transaction:

```text
stage source file
→ validate/parse
→ compute fingerprint
→ prepare metadata/cache
→ commit final book file
→ commit new Library generation
→ create initial per-book state
→ clean temporary data
```

If the Library record was not committed, staged data must not appear as a valid Library item.

## 16. Transactional replace

A replace operation must not destroy the last valid book/state before the new content is known-good.

The transaction should preserve:

- old book file/state;
- new staged file;
- intended migration decision;
- recoverable commit ordering.

Exact filesystem operations will be adapted to the verified SD/filesystem stack.

## 17. Schema migration

Every persisted record includes `schema_version`.

Firmware startup should support:

- current schema;
- explicitly supported older schema versions;
- migration to a newer generation;
- refusal/recovery when a schema is newer than the firmware understands.

Migration must create a new valid generation before invalidating old data.

## 18. Corruption behavior

Subsystems fail independently where possible.

Examples:

- one corrupt per-book state file → that book may lose/recover state, but Library still opens;
- corrupt cover cache → regenerate;
- one invalid Library slot + one valid slot → use valid slot;
- both Library slots invalid → enter Library recovery/rescan path;
- corrupt settings → use safe defaults and preserve recoverable data where possible.

Corruption must not trigger automatic deletion of original book files.

## 19. Storage abstraction requirements

The platform storage layer must expose enough semantics for safe commits:

- open/read/write;
- flush/sync when supported;
- close;
- file existence/size;
- remove;
- rename/move when supported;
- directory creation/enumeration;
- free-space query;
- explicit error reporting.

The persistence layer owns generation logic; the platform layer owns filesystem mechanics.

## 20. Memory behavior

Persistence code must support bounded-memory operation.

Preferred behavior:

- encode/decode records incrementally where practical;
- avoid loading original book files into memory for persistence work;
- keep Library records compact;
- keep per-book state independent;
- avoid giant monolithic object graphs.

The exact upper bounds will be profiled on the target board.

## 21. Debug/export path

A future maintenance function may export normalized state as JSON for inspection.

That export is not authoritative persistence and must not become required for normal reading.

## 22. Decisions fixed by this document

- Reader v1 uses versioned CBOR persistence.
- SQLite is deferred.
- JSON is not the primary runtime store.
- Library, global settings and per-book state are separate persistence domains.
- Library is not rewritten on every page turn.
- Per-book state uses A/B generations for crash recovery.
- Persisted records carry schema and integrity metadata.
- Cover cache is disposable.
- Original book files are never deleted merely because metadata/state records are corrupt.
- The exact CBOR library and filesystem implementation remain framework/hardware decisions.
