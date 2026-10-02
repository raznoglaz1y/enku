# ENKU Pagination & Rendering Model

This document defines the baseline architecture for text layout, pagination and page rendering in ENKU Reader v1.

The goal is to produce stable, readable pages on a constrained e-paper device without tying pagination to any specific source format.

## 1. Core principle

Pagination operates on the normalized document model produced by the parser layer.

The pagination engine must not contain format-specific branching for EPUB, FB2 or TXT.

Input:

```text
NormalizedDocument
+ SemanticPosition
+ TypographySettings
+ Viewport
```

Output:

```text
RenderedPage
+ firstSemanticPosition
+ lastSemanticPosition
+ previousAnchor
+ nextAnchor
+ progress
```

## 2. Page is a derived object

A rendered page is not persistent identity.

It is a temporary layout result derived from:

- book content;
- semantic position;
- font;
- font size;
- line spacing;
- margins;
- alignment;
- orientation;
- viewport dimensions.

Changing any of those may change page boundaries.

Therefore ENKU must never store "page 153" as the authoritative reading position.

## 3. Layout pipeline

Conceptual layout flow:

```text
semantic anchor
→ load normalized section content
→ shape/measure text
→ split into lines
→ apply paragraph/block rules
→ fill viewport
→ produce page display list
→ produce next/previous semantic anchors
```

The exact implementation may change after profiling, but the pipeline separation is fixed.

## 4. Display list

The pagination engine should return a compact display list rather than drawing directly to the e-paper driver.

A display list may contain commands such as:

- draw text run;
- draw heading;
- draw rule/separator;
- draw simple inline image in future revisions;
- draw emphasis/strong variant.

Conceptually:

```text
DrawText(x, y, fontStyle, textRange)
DrawRule(x1, y1, x2, y2)
```

The UI/render layer can then rasterize the page into the framebuffer independently from the parser.

## 5. Typography input

Reader layout uses the effective typography settings after resolving global vs per-book overrides.

Reader v1 settings:

- font family;
- font size;
- line spacing;
- margins;
- alignment.

Presets remain:

- Spacious
- Comfortable
- Standard
- Compact
- Dense
- Custom

Any manual change switches the effective preset to Custom.

## 6. Font and shaping model

The current reading font target is Noto Sans.

The text engine must support Unicode text needed by ENKU's initial language scope, especially Latin and Cyrillic.

Text measurement must use actual glyph metrics from the rendering font. It must not estimate width from character count.

The exact shaping stack remains an implementation decision.

Reader v1 does not require complex browser-level typography, but the architecture should not prevent a stronger shaping engine later.

## 7. Word wrapping

Baseline line-breaking behavior:

1. prefer breaking on legal whitespace boundaries;
2. preserve punctuation with natural surrounding text;
3. never split UTF-8 byte sequences;
4. measure the actual rendered run before deciding fit;
5. if a single token exceeds the line width, use a controlled fallback rather than overflow.

For very long tokens such as URLs or malformed text, the fallback may split at a safe Unicode/codepoint boundary.

Hyphenation is not required for Reader v1.

It may be added later as a language-aware optional feature.

## 8. Whitespace normalization

The normalized document layer should remove source-format noise while preserving meaningful reading structure.

Pagination should treat:

- repeated ordinary spaces as collapsible where appropriate;
- paragraph boundaries as semantic;
- explicit line breaks as intentional;
- heading separation as intentional;
- leading/trailing source whitespace as non-semantic unless format rules say otherwise.

TXT-specific line semantics are handled by the parser/normalization layer before pagination.

## 9. Paragraph layout

A paragraph is laid out as a sequence of wrapped lines.

Reader v1 should support:

- paragraph spacing;
- optional first-line indentation if the effective reading style chooses it later;
- left alignment;
- centered alignment;
- right alignment;
- justified alignment if implementation quality is acceptable.

Justification must not be enabled merely because it exists in the UI. It should be used only if spacing quality is acceptable on the real rendering stack.

## 10. Headings

Headings are structural blocks.

Rules:

- heading may use a distinct weight/size;
- heading should not be stranded at the very bottom with no following body line when avoidable;
- if a heading plus at least one body line cannot fit, move the heading to the next page;
- large headings may start a new page if layout quality requires it.

Exact heading styles remain part of the typography/theme layer.

## 11. Widow/orphan-like behavior

Full desktop-publishing widow/orphan control is not required for Reader v1.

However, ENKU should use lightweight page-quality rules:

- avoid leaving a heading alone at page bottom;
- avoid a single first line of a paragraph at page bottom when practical;
- avoid a single last line of a paragraph at the top of the next page when practical;
- never create excessive blank space solely to satisfy these rules.

These are best-effort constraints, not absolute guarantees.

## 12. Block boundaries

The pagination engine should understand logical blocks rather than treating the book as one giant plain-text string.

Initial block handling:

- paragraph;
- heading;
- quote;
- list item;
- separator;
- explicit line break.

A block may be split across pages when appropriate.

Some blocks, such as a short heading or separator, should prefer to remain intact.

## 13. Lists

Reader v1 list support should be simple and robust.

Supported behavior:

- bullet lists;
- numbered lists;
- wrapped list-item text aligned after the marker;
- list item may continue onto the next page.

Deep/nested browser-style CSS list behavior is not required for Reader v1.

## 14. Quotes and emphasis

The normalized model may carry semantic emphasis:

- emphasis / italic;
- strong / bold;
- block quote.

If the available font/render stack supports the style cleanly, preserve it.

If a style variant is unavailable, readability takes priority over exact source styling.

## 15. Images in Reader v1

Text reading is the first priority.

Book images may be represented in the normalized document model, but Reader v1 should not depend on full inline-image support.

Initial policy:

- cover images are supported through the Library/cover pipeline;
- inline book images may be skipped, simplified or deferred if memory/layout cost is too high;
- image omission must not corrupt surrounding text flow.

Inline-image rendering can be promoted to a later milestone after the text engine is stable.

## 16. Semantic position mapping

Every laid-out text run must be traceable back to semantic source position.

This mapping allows ENKU to:

- save progress;
- create bookmarks;
- restore after orientation changes;
- restore after typography changes;
- open search results;
- navigate table-of-contents entries.

The display list therefore needs source-span metadata, even if that metadata is not visible.

## 17. Previous/next page navigation

The engine should support deterministic navigation in both directions.

Forward pagination is straightforward from the current page end anchor.

Backward pagination is more difficult and should not depend on storing every previous rendered page.

Potential implementation strategies may include:

- checkpoint anchors;
- section-local reverse layout;
- cached nearby page boundaries.

The exact strategy remains open until profiling.

What is fixed: Previous must return to a logically stable prior page, not approximately jump by character count.

## 18. Page cache

A small in-memory cache may hold nearby page layout results.

Likely candidates:

- current page;
- next page;
- previous page.

The exact cache depth depends on RAM/PSRAM measurements.

The cache is disposable. Semantic position remains authoritative.

## 19. Progress calculation

Progress must not be based on rendered page count.

Preferred model:

```text
normalized semantic offset / normalized document length
```

or the closest format-independent equivalent.

This keeps progress stable when typography or orientation changes.

Progress is approximate from a reader perspective, but should be monotonic and stable.

## 20. Orientation changes

On orientation change:

1. preserve the current semantic anchor;
2. update viewport;
3. discard incompatible page-layout cache;
4. repaginate around the anchor;
5. render the new page;
6. preserve logical reading position as closely as possible.

Raw pixel coordinates are never persisted.

## 21. Typography changes

Typography changes follow the same principle:

1. preserve semantic anchor;
2. update effective typography;
3. invalidate layout cache;
4. repaginate;
5. render around the same logical position.

The currently visible sentence/paragraph should remain as close as practical to the user's position.

## 22. Search highlight

Search matches are semantic ranges, not pixel coordinates.

When a match is opened:

- resolve its semantic position;
- paginate the corresponding page;
- map the match range into the page display list;
- render a monochrome-safe highlight such as underline, weight or inversion.

The highlight must remain visible without relying on color.

## 23. Rendering and e-paper

The pagination engine does not decide display refresh mode.

It only produces the page content/display list.

The UI/render layer converts that into a framebuffer/update region.

The Refresh Manager then decides partial vs full refresh.

This keeps page layout independent from panel behavior.

## 24. Performance strategy

Reader v1 should favor bounded work.

Guidelines:

- do not paginate the entire book in advance;
- paginate on demand around the current position;
- keep small nearby page caches;
- cache metadata and covers separately;
- avoid whole-book text copies in RAM;
- process sections incrementally;
- reuse buffers where practical.

PSRAM may be used where available, but the architecture should not assume unlimited memory.

## 25. Failure behavior

If layout fails because of malformed content or resource limits:

- preserve the last valid semantic position;
- show a product-level error;
- do not corrupt stored progress;
- allow Back to return safely to the Library.

Raw layout diagnostics can be logged separately.

## 26. Reader v1 typography scope

Required:

- Noto Sans baseline;
- Latin and Cyrillic;
- wrapping;
- headings;
- paragraph spacing;
- bold/strong where available;
- italic/emphasis where available;
- alignment;
- five presets + Custom;
- global and per-book settings.

Deferred unless implementation proves inexpensive:

- language-aware hyphenation;
- advanced kerning/shaping beyond the chosen engine's baseline;
- drop caps;
- floats;
- complex CSS;
- multi-column layout;
- footnote popovers;
- full inline-image fidelity.

## 27. Decisions fixed by this document

- Pagination is format-independent.
- Pages are derived/cached objects, not persistent identity.
- Semantic position is authoritative.
- Layout returns a display list rather than drawing to hardware directly.
- Width measurement uses real glyph metrics.
- Reader v1 does not require hyphenation.
- Lightweight heading/widow/orphan quality rules are used.
- Progress does not depend on page count.
- Orientation and typography changes repaginate around the semantic anchor.
- Rendering is separate from e-paper refresh policy.
- Full-book pre-pagination is not a Reader v1 requirement.
