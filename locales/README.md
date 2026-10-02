# ENKU Locales

English is the canonical source language for the ENKU on-device UI.

Planned Reader v1 locale source files:

```text
en.json
pl.json
de.json
fr.json
es.json
it.json
ru.json
```

The actual string catalog will be added once the canonical EN screen copy is finalized.

Rules:

- English defines the complete key set.
- Translation files may not introduce independent UI semantics.
- Named placeholders must match the English source.
- Runtime firmware should consume generated compact lookup data rather than parse these JSON files on every device boot.
- Missing translations fall back to English.

See [Localization Architecture](../docs/localization-model.md).
