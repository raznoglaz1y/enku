# ENKU Library & Storage Model

This document defines the baseline data model for books, Library indexing, progress, bookmarks, cover cache and transactional import.

The goal is to keep book files independent from reader state, avoid reparsing the whole Library on every boot, and preserve progress even when display layout changes.

## 1. Storage layout

Proposed logical layout:

```text
/books/
  <book files>

/system/
  library.db
  settings.db

/system/covers/
  <cached cover assets>

/system/state/
  <per-book state>

/system/tmp/
  <incomplete imports and temporary files>
```

The exact database/file format is not fixed yet, but the separation of responsibilities is.

## 2. Book files

Original book files remain separate from ENKU metadata/state.

Reader v1 formats:

- EPUB
- FB2
- TXT

The original file should not be modified just to store ENKU progress, bookmarks or per-book settings.

## 3. Library index

ENKU should maintain a persistent Library index so the UI does not need to parse every book on every startup.

The in-memory Library records, query/filter/sort model and service boundary are defined in [Library Data Model & Service Interface](library-model.md).

Each Library entry should include at least:

- `book_id`
- source file path
- source filename
- format
- file size
- file fingerprint
- title
- author
- language when available
- cover-cache reference
- reading state
- reading progress
- last-opened timestamp/order
- current semantic reading position
- import/version metadata needed for migration

Optional format-specific metadata may be added later.

## 4. Stable book identity

`book_id` must not be derived only from the filename.

Renaming or moving a file must not automatically destroy reading progress.

ENKU should use a stable internal identifier backed by a content-derived fingerprint.

Conceptually:

```text
fingerprint = hash(relevant file/content identity)
book_id = internal stable identifier associated with that fingerprint
```

The current import MVP uses FNV-1a 64 plus byte length as a deterministic provisional content fingerprint. It is isolated behind `BookImportService` and is not treated as a cryptographic identity guarantee; migration to SHA-256 can happen without changing the Library API.

### Duplicate detection

Duplicate detection should be stronger than filename comparison.

Import should consider:

- fingerprint/content identity;
- normalized metadata;
- file size as a supporting signal;
- format-specific identifiers when trustworthy.

Filename alone is never authoritative.

## 5. Reading state

Canonical Library states:

- **New**
- **Reading**
- **Finished**

Suggested behavior:

- imported and never opened → New;
- opened with progress below completion → Reading;
- completed → Finished;
- restarting a finished book → Reading;
- resetting progress does not delete the book.

Progress shown in the UI is derived from the semantic reading position / normalized book length, not from an unstable rendered page number.

## 6. Semantic reading position

Per-book progress must store a logical content anchor, not a rendered page number.

Conceptually:

```text
book_id
+ structural location
+ text/character offset or equivalent anchor
```

This allows the reader to preserve position across:

- font-size changes;
- line-spacing changes;
- margin changes;
- portrait/landscape changes;
- pagination fixes.

## 7. Bookmarks

A bookmark should include:

- bookmark id
- `book_id`
- semantic reading position
- chapter/section title when available
- short text snippet for UI display
- creation order or timestamp

Bookmarks must remain valid after repagination.

They must not be stored as page numbers.

## 8. Per-book settings

Per-book settings are stored separately from the book file.

Examples:

- typography preset / Custom
- font size
- line spacing
- margins
- alignment
- future reader-specific overrides

If no per-book override exists, global reading settings apply.

Clearing per-book settings must not change the raw book file, progress or bookmarks.

## 9. Cover cache

Cover rendering should use a cache rather than repeatedly decoding the source asset.

The cache may contain:

- normalized/cropped preview for Library;
- reader-sleep cover variant if needed;
- metadata linking the cache to the source book fingerprint.

If source metadata or the book file changes, the cache can be invalidated and regenerated.

Missing or invalid covers use a deterministic fallback.

## 10. Transactional import

Import must be transactional.

Canonical flow:

```text
receive/copy
→ temporary file
→ validate format
→ extract metadata
→ compute fingerprint
→ check duplicates
→ prepare cover/cache
→ commit final book file
→ commit Library entry
→ remove temporary data
```

The first framework-neutral `BookImportService` now implements the validation/indexing core for TXT sources supplied as bytes:

```text
detect format
→ compute deterministic content fingerprint
→ reject exact duplicate
→ parse/validate TXT
→ normalize metadata
→ build BookRecord
→ LibraryService.upsert()
```

The source-file staging/copy step remains a platform/storage concern and will wrap this core pipeline later. EPUB/FB2 imports remain unsupported until their parsers exist.

The initial per-book state is represented by absence of a checkpoint. This is intentional: `ReaderCheckpointService::load() == NotFound` already means start from the beginning, so import does not create an unnecessary state write or risk a partially committed Library/state transaction.

A book must not appear in the Library before the commit stage succeeds.

## 11. Import failure / power loss

Incomplete imports live under a temporary area and are never treated as valid Library books.

After reboot, ENKU should:

1. inspect the temporary import area;
2. remove or recover only clearly valid staged operations;
3. keep the existing Library database consistent;
4. never delete a previously valid book just because an update/import failed.

## 12. Safe replace

Replacing an existing book should also be transactional.

Preferred flow:

```text
new temp file
→ validate
→ fingerprint
→ compare with existing book
→ determine whether reader state can be preserved
→ commit replacement
→ migrate/preserve state when safe
→ invalidate/regenerate metadata/cover cache
→ remove old file only after successful commit
```

If the replacement is unrelated content, ENKU must not silently attach the old progress/bookmarks to the new book.

## 13. Delete behavior

Deleting a book removes:

- the local book file;
- its Library record;
- ENKU reading progress;
- bookmarks;
- per-book settings;
- cached cover assets.

A failed delete should leave a recoverable Library state rather than pretending the operation succeeded.

## 14. Library startup

On normal startup, ENKU should load the Library index first rather than parsing every file.

A background or explicit rescan may reconcile:

- files added manually to storage;
- files removed outside ENKU;
- changed files;
- stale cache entries.

The exact rescan strategy will be chosen after storage performance is measured.

## 15. Persistence backend

Reader v1 uses **versioned CBOR records with A/B generation recovery**.

The detailed persistence strategy is defined in [Persistence Backend](persistence-model.md).

Key decisions:

- no SQLite dependency in Reader v1;
- JSON is reserved for optional debug/export tooling rather than authoritative runtime storage;
- Library index, global settings and per-book state are separate persistence domains;
- progress checkpoints do not rewrite the full Library index;
- each critical record carries schema/generation/integrity metadata;
- recovery chooses the newest valid generation;
- cover cache remains disposable.

## 16. Decisions fixed by this document

- Library uses a persistent index.
- Book identity is not filename-based.
- Duplicate detection uses content/fingerprint signals.
- Progress and bookmarks use semantic positions.
- Raw books are separate from ENKU state.
- Covers are cached.
- Import and replacement are transactional.
- Incomplete imports never appear as valid Library entries.
- Per-book settings are separate from global settings and raw book files.
- Runtime persistence uses versioned CBOR records with generation-based recovery.


## 17. Staged file import MVP

A concrete staged import flow now exists above the framework-neutral import core.

`StagedBookImportService` coordinates `BookFileStore`, `BookImportService` and `LibraryService`.

Current transaction:

```text
/system/tmp/<upload>
→ read staged bytes
→ BookImportService.prepare()
→ validate / fingerprint / duplicate check / parse
→ derive canonical /books/book-<id>.<ext>
→ write final book file
→ BookImportService.commit()
→ persist Library index
→ remove staged upload
```

Failure behavior:

- missing/unreadable staged file → no Library or final-book changes;
- unsupported/invalid/duplicate content → staged file remains for deterministic UI handling;
- final book write failure → Library is unchanged and staged file remains;
- Library commit failure → newly written final book file is removed and staged file remains for retry;
- staged cleanup failure after commit → the imported book remains valid and the result reports `CleanupFailed` so recovery can remove the leftover tmp file later.

The final filename is content-identity based rather than source-filename based:

```text
/books/book-<fingerprint-derived-id>.<ext>
```

This avoids collisions between unrelated uploads that happen to share a filename.

The POSIX implementation is exercised by host integration tests using real temporary files. The target ESP32/SD adapter will implement the same `BookFileStore` contract.
