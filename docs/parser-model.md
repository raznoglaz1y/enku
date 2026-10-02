# ENKU Metadata & Parser Model

This document defines how EPUB, FB2 and TXT are normalized into one internal book model for ENKU.

The goal is to keep format-specific parsing isolated from the Library, Reader UI and persistence layers.

## 1. Parser abstraction

Each supported format implements the same logical parser contract.

Conceptually:

```text
BookParser
  canOpen(file)
  inspect(file) -> BookMetadata
  open(file) -> BookDocument
  resolvePosition(anchor) -> SemanticPosition
  readSection(sectionId)
  search(query)
```

The exact C++ API is not fixed yet, but the separation is.

The Library and Reader Engine should work with normalized ENKU structures rather than EPUB-, FB2- or TXT-specific objects.

The Library-facing record/query representation is defined separately in [Library Data Model & Service Interface](library-model.md).

## 2. Canonical metadata model

Every imported book is normalized into a common metadata record.

Required ENKU fields:

- `book_id`
- `format`
- `title`
- `author_display`
- `source_path`
- `source_filename`
- `file_size`
- `fingerprint`

Optional normalized fields:

- `authors[]`
- `language`
- `description`
- `series_name`
- `series_index`
- `publisher`
- `published_date`
- `identifier`
- `cover_source`
- `toc_available`

Format-specific metadata may exist internally, but UI code should not depend on it.

## 3. Metadata fallback rules

Missing metadata must not block import.

### Title

Priority:

1. embedded book title;
2. normalized source filename without extension;
3. `Untitled book` as a final fallback.

### Author

Priority:

1. embedded author/creator metadata;
2. multiple authors joined into a display string;
3. `Unknown author`.

The raw authors list should still be preserved when the format provides it.

### Language

Priority:

1. embedded language metadata;
2. parser-supported language inference only if later proven reliable;
3. unknown / unset.

ENKU must not invent a language value merely from the UI language.

### Description

If unavailable, the field remains empty. The UI should omit the description block rather than display fake placeholder text.

### Cover

Priority:

1. embedded cover;
2. user-supplied replacement cover;
3. deterministic ENKU fallback cover.

## 4. User-edited metadata

User corrections made through the local web interface must not rewrite the source book unless ENKU explicitly adds such a feature later.

Instead, ENKU stores normalized metadata overrides.

Conceptually:

```text
source metadata
+ user overrides
= effective metadata
```

This makes edits reversible and keeps original files intact.

Supported first-version overrides should include:

- title;
- author display;
- language;
- cover.

Description/series editing may be added later if useful.

## 5. Normalized document model

After opening a book, each parser exposes a common logical document structure.

Conceptually:

```text
BookDocument
  metadata
  sections[]
  tableOfContents[]
  resources[]
```

Each section has:

- stable section id;
- logical order;
- normalized text/content stream;
- optional heading/title;
- position mapping information.

The Reader Engine paginates this normalized representation.

## 6. Text model

The first reader implementation should focus on a compact set of semantic text blocks.

Initial block types:

- paragraph;
- heading;
- line break;
- emphasis;
- strong/bold;
- block quote;
- list item;
- separator.

Images inside books may be represented in the normalized model, but inline-image rendering is not required to block the first text-focused Reader MVP.

Advanced CSS/layout behavior is intentionally outside the first implementation.

## 7. EPUB normalization

EPUB parser responsibilities:

- open the ZIP container;
- locate package metadata;
- read spine order;
- normalize metadata;
- build ordered reading sections;
- extract table of contents when available;
- resolve internal text anchors;
- locate/extract cover where possible;
- ignore unsupported presentation features safely.

Reader v1 should prioritize readable reflowable text over full browser-level EPUB/CSS fidelity.

## 8. FB2 normalization

FB2 parser responsibilities:

- parse XML safely;
- extract title-info metadata;
- normalize authors;
- read language and sequence/series information where available;
- build ordered sections from body structure;
- derive table of contents from section titles;
- extract referenced cover image where available;
- preserve stable section/text offsets for semantic positions.

FB2-specific XML details stay inside the parser layer.

## 9. TXT normalization

TXT has little or no embedded metadata.

A first framework-neutral TXT parser MVP now exists under `firmware/src/parsers/txt/`.

Current MVP behavior:

- accepts UTF-8 and UTF-8 with BOM;
- rejects UTF-16 BOM input as unsupported rather than guessing;
- validates UTF-8;
- normalizes CRLF/CR to LF;
- uses filename stem as title fallback;
- uses `Unknown author`;
- groups text separated by blank lines into paragraph blocks;
- exposes one stable section id: `txt:body`;
- tracks normalized UTF-8 byte offsets for source-position mapping.

Additional encoding support and file/stream integration remain implementation work.

TXT parser responsibilities:

- detect/normalize supported text encoding;
- normalize line endings;
- use filename fallback for title;
- use `Unknown author` unless metadata is supplied externally;
- build one logical document stream;
- optionally identify simple chapter-like headings only if the heuristic is conservative.

ENKU must not invent a complex table of contents from arbitrary text.

## 10. Encoding and Unicode

Internally, ENKU should normalize text to UTF-8.

Reader v1 must handle at least the scripts needed by the planned UI/book usage, including Latin and Cyrillic.

Invalid byte sequences should be handled gracefully:

- reject the file if decoding is impossible;
- or replace invalid sequences in a controlled way when safe.

The exact decoder libraries and normalization strategy remain implementation choices.

## 11. Table of contents

The parser layer exposes a normalized table of contents:

```text
TocEntry
  id
  title
  level
  semanticPosition
```

EPUB and FB2 may provide structural ToC data directly.

TXT may have no ToC.

Missing ToC is a valid state and must not prevent reading.

## 12. Search model

Search operates on normalized text, not rendered pages.

A result should contain:

- semantic position;
- section/chapter context when available;
- short text snippet;
- match range.

This keeps search results stable across typography/orientation changes.

## 13. Parser failure model

Parser errors should be classified into product-meaningful categories rather than shown as raw implementation errors.

Examples:

- unsupported format;
- invalid/corrupt file;
- unreadable archive/container;
- invalid XML;
- unsupported/unknown text encoding;
- missing required document structure;
- resource/size limit exceeded;
- internal parser error.

The raw diagnostic may be logged separately for development.

## 14. Resource limits

ESP32-S3 memory limits mean parsers should prefer streaming/incremental work instead of loading an entire book into RAM.

Implementation should aim for:

- bounded parsing buffers;
- section-level processing;
- lazy resource decode;
- cached metadata/cover;
- no permanent full-book DOM unless measurements prove it practical.

Exact limits will be determined by profiling on the target board.

## 15. Format-independent Reader Engine

The Reader Engine should consume the same normalized interfaces regardless of source format.

That means the pagination layer should not contain logic such as:

```text
if EPUB ...
if FB2 ...
if TXT ...
```

Format branching belongs in the parser/adaptation layer.

## 16. Decisions fixed by this document

- EPUB, FB2 and TXT use a shared parser abstraction.
- Metadata is normalized before it reaches Library/UI code.
- Missing metadata never blocks import by itself.
- Filename is a title fallback, not the primary identity.
- User metadata corrections are stored as overrides, leaving source files intact.
- Reader content is normalized into format-independent sections/text blocks.
- Search and ToC use semantic positions.
- Internal text representation is UTF-8.
- Parser work should be streaming/bounded-memory where practical.
- Full browser-level EPUB/CSS fidelity is not a Reader v1 goal.
