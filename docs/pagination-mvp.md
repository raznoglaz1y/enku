# ENKU Pagination MVP

This document describes the first host-testable pagination implementation built on top of the normalized TXT parser path.

The broader pagination architecture remains defined in [Pagination & Rendering Model](pagination-model.md).

## Current MVP path

```text
TXT bytes
→ TxtParser
→ BookDocument
→ TextPaginator
→ PaginationPage
→ next semantic anchor
```

The implementation is intentionally independent from the e-paper driver and from a specific font library.

## Text measurement boundary

Pagination does not estimate text width from character count.

It consumes a `TextMeasurer` interface supplied by the eventual font/render stack:

```text
measureWidthPx(utf8, typography)
lineHeightPx(typography)
```

Host tests use a deterministic fixed-width fake measurer only to validate pagination behavior.

Production rendering must provide real Noto Sans glyph metrics.

## Inputs

The paginator consumes:

- normalized `BookDocument`;
- semantic anchor;
- typography settings;
- viewport;
- text measurer.

## Outputs

The first MVP returns:

- visible `PageLine[]`;
- first semantic position;
- last semantic position;
- optional `next_anchor`;
- normalized progress.

Page numbers are not generated or persisted.

## Current behavior

Implemented:

- forward pagination;
- paragraph text wrapping;
- word-boundary preference;
- safe UTF-8 codepoint fallback for tokens wider than a line;
- viewport margins;
- measured line height;
- semantic byte-offset mapping;
- section-aware anchors;
- bounded page construction;
- end-of-document detection.

Not yet implemented:

- deterministic previous-page reconstruction;
- headings/list/quote-specific layout;
- paragraph spacing;
- widow/orphan quality rules;
- justification;
- hyphenation;
- inline images;
- real Noto Sans metrics;
- page cache/checkpoints.

## Semantic offsets

The TXT parser currently records normalized UTF-8 byte offsets.

The paginator returns anchors in the same normalized document coordinate system.

This is sufficient for the first TXT pipeline, but EPUB/FB2 adapters may need richer structural anchors while preserving the same `SemanticPosition` product contract.

## Next integration point

After hardware/font bring-up:

1. connect a real font-backed `TextMeasurer`;
2. render returned lines into a framebuffer/display list;
3. send visible changes through the Refresh Manager;
4. validate typography and page boundaries on the real 800×480 panel.

## Decisions fixed by this MVP

- Pagination depends on measured glyph/text width through an abstraction.
- Host tests may use fake metrics; production code may not.
- The first implementation paginates on demand instead of pre-paginating a whole book.
- Semantic anchors remain authoritative.
- TXT is the first complete parser → pagination path.
