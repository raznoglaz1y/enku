# ENKU Localization Architecture

This document defines the Reader v1 localization model for the on-device UI.

English is the canonical source language. Translations are generated from the English string set and must never become independent UI specifications.

## 1. Initial locale set

Reader v1 targets:

- English (`en`)
- Polish (`pl`)
- German (`de`)
- French (`fr`)
- Spanish (`es`)
- Italian (`it`)
- Russian (`ru`)

English is always available and is the fallback locale.

The interface locale is independent from:

- book language;
- keyboard input mode;
- book encoding;
- metadata language.

## 2. Stable string keys

UI code must never embed production user-facing text directly in screens.

Every UI string is referenced by a stable semantic key.

Examples:

```text
library.title
library.empty.title
library.empty.body
reader.menu.contents
reader.menu.bookmarks
settings.language
power.sleep
power.power_off
import.progress
```

Keys describe meaning, not English wording.

Bad:

```text
"Tap_to_continue"
"Delete_book_question"
```

Good:

```text
setup.continue
book.delete.confirm_title
```

Changing English copy must not require changing the key unless the semantic meaning changes.

## 3. Source format

Repository translation sources should remain human-reviewable UTF-8 text files.

Recommended source layout:

```text
locales/
  en.json
  pl.json
  de.json
  fr.json
  es.json
  it.json
  ru.json
```

English defines the canonical key set.

Non-English source files may not introduce keys that do not exist in English.

The exact source serialization can still change if tooling proves a better option, but the canonical-key model is fixed.

## 4. Runtime representation

Human-readable source files are a build-time authoring format, not necessarily the runtime representation.

The build pipeline should convert locale sources into compact lookup data suitable for the ESP32-S3.

Preferred runtime properties:

- UTF-8;
- deterministic key/index mapping;
- no JSON parser required for normal UI rendering;
- no per-string heap allocation required just to look up text;
- compact tables/blobs;
- English fallback always available.

The exact generated binary/table representation remains implementation-specific.

## 5. English fallback

Lookup sequence:

```text
requested locale
→ English
→ visible missing-string marker in development builds
```

A missing translated string must not produce an empty control.

Release builds should fall back to English.

Development builds should additionally log missing keys.

## 6. Locale identifiers

Firmware uses stable locale identifiers rather than arbitrary strings throughout application logic.

Conceptually:

```text
LocaleId::En
LocaleId::Pl
LocaleId::De
LocaleId::Fr
LocaleId::Es
LocaleId::It
LocaleId::Ru
```

BCP-47-style tags remain available for persistence/import/export where needed.

## 7. Variables

Dynamic strings use **named placeholders**.

Example source:

```text
"storage.free_space": "{free} free"
"import.books_added": "{count} books added"
```

Do not use positional formatting such as:

```text
"%s of %s"
"%d books"
```

Named placeholders make translations safer because word order can change.

## 8. Formatting responsibilities

Localization owns linguistic formatting.

Application code provides semantic values.

Example:

```text
UI:
  tr("import.books_added", count=3)

Localization:
  chooses locale form
  inserts localized number/value
```

Screen code must not concatenate sentence fragments such as:

```text
tr("books") + " " + count
```

## 9. Plurals

Reader v1 needs a small explicit plural system rather than a full ICU MessageFormat implementation.

The locale layer maps a numeric count to a plural category.

Supported categories:

- `one`
- `few`
- `many`
- `other`

Not every language uses every category.

Pluralized keys conceptually use variants:

```text
import.books_added.one
import.books_added.few
import.books_added.many
import.books_added.other
```

Locale-specific plural rules are implemented centrally and tested independently.

Screen code sees only the logical message plus `count`.

## 10. Numbers and percentages

Reader v1 keeps numeric formatting intentionally small.

Localization may format:

- integer counts;
- percentages;
- storage sizes;
- simple dates/times where the UI requires them.

Do not introduce a heavyweight internationalization framework solely for number formatting.

Formatting rules should be centralized so UI screens do not each invent their own representation.

## 11. Dates and time

The status-bar clock format and date presentation are locale-aware product settings.

The first implementation may use a deliberately small formatting layer.

Requirements:

- 24-hour/12-hour behavior must not be inferred from book language;
- persisted time values are locale-neutral;
- formatted strings are produced only at presentation time.

Exact regional time-format preferences remain a later product decision.

## 12. Font coverage

Localization and reading text are separate concerns, but UI glyph coverage must be validated for all shipping locales.

Current UI/reading font target: **Noto Sans**.

The font assets used by firmware must contain the glyphs required for:

- English;
- Polish;
- German;
- French;
- Spanish;
- Italian;
- Russian.

Font subsetting is allowed only if it preserves every required UI glyph.

CJK is outside the initial localization wave.

## 13. Memory strategy

Seven locales must not result in seven copies of unrelated UI structures.

Only strings differ.

Preferred strategy:

- one shared UI layout/component implementation;
- stable numeric/string-key table;
- compact per-locale text blobs/tables;
- English always present;
- other first-wave locales packaged in firmware if measured flash budget permits.

If flash profiling shows that all locales materially constrain the reader, optional locale packs may be evaluated later.

Reader v1 must not depend on network access to obtain its selected UI language.

## 14. Long-string constraints

Translations must not be made artificially short by shrinking typography.

Existing UI rules remain:

- screen/list titles may use up to two lines where specified;
- metadata/support text generally uses one line + ellipsis;
- buttons retain approved typography;
- controls do not reduce font size to fit;
- layout is tested in both 800×480 and 480×800.

Canonical EN boards must be stress-tested at least with long German, French and Polish strings.

## 15. Translation quality rules

Translations should preserve product meaning, not copy English grammar.

Rules:

- translate actions as actions;
- keep terminology consistent across screens;
- avoid unexplained abbreviations;
- do not translate brand/product name **ENKU**;
- do not translate file extensions such as EPUB/FB2/TXT;
- preserve placeholders exactly;
- preserve semantic distinctions such as Sleep vs Power Off;
- preserve focus/selection terminology in implementation documentation.

## 16. First-start language selection

First Start begins with language selection.

The language list should display each language in its own familiar name where practical while keeping deterministic navigation.

Changing language:

- updates UI strings immediately after confirmation;
- does not change keyboard input mode automatically;
- does not change book language;
- persists the selected `LocaleId` through `SettingsRuntimeController` / `CborSettingsService`.

## 17. Runtime language change

Settings → Language can change UI locale without reboot.

Canonical flow:

```text
Language selected
→ persist LocaleId
→ invalidate visible localized text layout
→ rebuild current screen
→ refresh affected UI
```

Reader semantic position does not change.

If a translated label changes layout, focus remains on the same logical control.

## 18. Error messages

Internal diagnostics and user-facing errors are separate.

User-facing errors use localization keys.

Developer logs may remain English.

Raw parser/filesystem/driver error strings should not be displayed directly as normal product copy.

## 19. Web management UI

The local web-management interface should use the same canonical terminology but may have its own translation bundle.

On-device locale and browser locale do not have to be coupled.

The first web UI can remain English-only if needed, provided the on-device locale system is not architecturally tied to that decision.

## 20. Validation tooling

The localization build/check step should verify:

- every non-English key exists in English;
- no duplicate keys;
- placeholder names match the English source;
- plural variant sets are valid for that message;
- UTF-8 validity;
- missing translations;
- optionally, maximum/reference string lengths for review reports.

Missing translations should be visible in CI/development reports even though runtime fallback is English.

## 21. Persistence

The selected locale is stored as a stable locale identifier in global settings.

Persistence must not store translated labels.

Correct:

```text
locale = "pl"
sort = "author"
```

Incorrect:

```text
locale = "Polski"
sort = "Autor"
```

This keeps persistent state independent from copy changes.

## 22. Decisions fixed by this document

- English is the canonical and fallback locale.
- Reader v1 targets EN/PL/DE/FR/ES/IT/RU.
- UI code uses stable semantic string keys.
- Production screens do not embed user-facing copy directly.
- Named placeholders are required.
- Sentence-fragment concatenation is prohibited.
- A lightweight centralized plural-category layer is used.
- Runtime localization does not require a JSON parser.
- Language changes do not change keyboard mode or book language.
- Locale selection is persisted as a stable identifier.
- Development tooling reports missing keys/placeholders.
- The exact source-file generator and runtime table implementation remain implementation details.
