# ENKU Library Data Model & Service Interface

This document defines the Reader v1 in-memory Library model and the interface used by UI/runtime code to query and update books.

It builds on the storage, metadata and persistence models without exposing CBOR/filesystem details to the UI.

## 1. Core principle

The Library is a logical service.

UI code asks the Library for normalized book records and query results.

It must not:

- scan directories directly;
- parse EPUB/FB2/TXT metadata directly;
- decode persistence files;
- compare raw filenames to decide book identity;
- modify progress/bookmarks by editing book files.

Conceptually:

```text
UI / Runtime
→ LibraryService
→ Library index + per-book state
→ Persistence / Storage
```

## 2. Book format

Reader v1 has three source formats:

- EPUB
- FB2
- TXT

The format is stored as a stable enum/value, not inferred repeatedly from the filename after import.

## 3. Normalized metadata

`BookMetadata` contains normalized presentation metadata.

Core fields:

- title;
- author display string;
- authors list;
- language when known;
- description when known;
- series name/index when known;
- publisher when known;
- published date when known;
- source identifier when trustworthy;
- table-of-contents availability;
- cover state/reference.

Metadata values may come from:

```text
source metadata
+ ENKU fallback rules
+ user overrides
= effective metadata
```

The UI consumes effective metadata and does not need to know the parser source.

## 4. Book record

A `BookRecord` represents one indexed Library item.

It includes:

- stable `book_id`;
- format;
- effective metadata;
- source path;
- source filename;
- source size;
- source fingerprint;
- reading state;
- normalized progress;
- last-opened value/order;
- cover-cache reference when available.

The Library record is not the entire per-book state.

Bookmarks and detailed semantic reading position remain in the per-book state persistence domain.

## 5. Summary vs detailed state

Library list/grid rendering should not require loading every bookmark or complete reader state.

The index therefore carries only summary fields needed for Library presentation and sorting:

- reading state;
- progress;
- last opened;
- cover reference;
- title/author.

Detailed per-book state is loaded on demand when:

- opening the book;
- showing Book Details where needed;
- accessing bookmarks;
- modifying per-book typography/settings.

## 6. Reading states

Canonical states remain:

- **New**
- **Reading**
- **Finished**

State transitions:

```text
imported, never opened → New
opened / active progress → Reading
completed → Finished
restart finished book → Reading
reset progress → New or Reading according to explicit product action
```

The exact reset-product wording is a UI concern; state transitions remain explicit rather than inferred from a rendered page number.

## 7. Library filters

Reader v1 browse filters:

- All
- New
- Reading
- Finished

Filtering operates on canonical reading state.

A missing/corrupt book may additionally carry an availability/error state internally, but it is not silently mapped to one of the reading-state filters.

## 8. Library sorting

Initial logical sort modes:

- Title
- Author
- RecentlyOpened
- RecentlyAdded

Sort direction is explicit where relevant.

Stable tie-breaking should use a deterministic field such as `book_id` so repeated queries do not reorder equal items unpredictably.

Exact locale-aware collation quality will depend on the selected runtime text stack; Reader v1 must at minimum provide deterministic UTF-8-safe ordering.

## 9. Global search

Library search is global across:

- effective title;
- effective author display / authors.

Per the UI specification, search is **not constrained by the currently selected Library reading-state filter**.

The query model therefore distinguishes:

- Browse query — filter + sort;
- Search query — global title/author search + sort.

Search must not modify the persisted active Library filter.

## 10. Query paging

A large Library must not require building an unbounded UI result set in RAM.

Library queries therefore support bounded paging:

```text
offset
limit
total_matches
items[]
```

The exact page size is selected during implementation/profiling.

UI may request additional result pages as focus/scroll approaches the current range.

## 11. Focus persistence

Library UI persists logical focus by `book_id`, not by raw row number.

When returning to Library:

1. re-run current Browse query;
2. attempt to locate the previously focused `book_id`;
3. restore it if still present;
4. otherwise select the nearest deterministic valid item.

This survives insertions, deletes and sort changes better than storing a row index.

## 12. Book lookup

`LibraryService` exposes direct lookup by stable `book_id`.

Book lookup returns no record when:

- id is unknown;
- entry has been deleted;
- index has not recovered it.

Storage availability is a separate condition and should be reported explicitly where needed.

## 13. Updates

Library updates are explicit operations.

Examples:

- add committed import;
- remove book;
- replace source metadata/file reference;
- update effective metadata override;
- update summary progress/state;
- update last-opened value;
- update cover-cache reference.

The service implementation decides which persistence domain must be committed.

UI does not manually edit the Library CBOR representation.

## 14. Progress synchronization

Current semantic position lives in per-book state.

The Library index stores only summary progress/state needed for Library UI.

When progress is checkpointed:

1. commit/update per-book state as required;
2. update Library summary in memory;
3. persist Library summary according to its lower-frequency policy.

The implementation may debounce Library-summary persistence separately to avoid rewriting the index for every page turn.

## 15. Metadata overrides

Metadata edit operations update override data, not the original book file.

After an override:

- effective metadata changes;
- relevant sort/search indexes/results change;
- visible Library rows/cards are invalidated;
- source fingerprint remains source-content identity and does not change merely because a title/author override changed.

## 16. Duplicate lookup

Import code may ask the Library whether a source fingerprint already exists.

The Library service may provide:

- exact fingerprint lookup;
- format/source identifier lookup where trustworthy;
- metadata similarity helpers later if needed.

Exact-byte/source fingerprint equality is authoritative for exact duplicate detection.

Fuzzy "same book" matching is not part of the basic Library query API.

## 17. Missing source file

An indexed record may temporarily exist while its source file is unavailable.

Examples:

- removable storage not mounted;
- file removed externally;
- storage error.

The Library must not immediately delete the record/state.

Instead it can report availability separately and allow reconciliation/retry.

## 18. Rescan / reconciliation

A rescan reconciles index and filesystem without bypassing transactional rules.

Possible findings:

- new unindexed file;
- indexed file missing;
- fingerprint changed;
- stale cover cache;
- duplicate file.

Rescan results should be applied deliberately rather than rebuilding the entire Library destructively from filenames.

## 19. Service boundary

Conceptual Reader v1 interface:

```text
LibraryService
  get(book_id)
  query(LibraryQuery)
  count(LibraryQuery)
  findByFingerprint(...)
  updateSummary(...)
  updateMetadataOverride(...)
  remove(...)
  rescan(...)
```

Exact C++ return/result wrappers may evolve with the selected framework, but this boundary is fixed.

## 20. Failure behavior

Library operations return structured status/errors.

Examples:

- NotFound
- StorageUnavailable
- InvalidRecord
- PersistenceFailure
- NoSpace
- Busy
- InvalidQuery

A query failure must not be represented as an empty Library.

This distinction prevents storage/persistence errors from appearing as "you have no books".

## 21. Memory behavior

Library implementation should:

- load compact index data rather than parse every book at boot;
- keep bounded query result pages;
- lazy-load detailed book state;
- avoid decoding cover files just to sort/search;
- avoid retaining full parser documents for Library records.

## 22. Decisions fixed by this document

- Library UI/runtime access goes through `LibraryService`.
- Book identity uses stable `book_id`, never row number or filename.
- Library records contain summary state, not full bookmarks/reader state.
- Browse filters are All/New/Reading/Finished.
- Initial sorts are Title/Author/RecentlyOpened/RecentlyAdded.
- Search is global across title/author and ignores the active browse filter.
- Query results are bounded/paged.
- Focus persistence uses `book_id`.
- Metadata overrides do not change source fingerprint identity.
- Missing source files do not automatically delete Library/state.
- Query/storage failures are distinct from an empty Library.
