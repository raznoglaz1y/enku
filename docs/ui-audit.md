# ENKU UI audit

This document tracks the design audit of the current screen baseline before firmware implementation.

Status values:
- **Approved** — usable as an implementation reference with only localization/content replacement.
- **Needs revision** — concept is valid but the board must be updated before implementation.
- **Reference only** — useful as a design/system reference, not a production screen.
- **Obsolete** — should not be implemented.

## Block 1 — First start / language / orientation

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_first_start_language_v1` | **Needs revision** | Flow is good, but EN is now the source/reference language. Expand the selector from RU/EN to EN/RU/PL/DE/FR/ES/IT. Language names should be shown in their own language. The selected row and focus row must remain visually distinct. Re-check portrait height once all languages are present. |
| `ENKU_first_start_orientation_v1` | **Approved** | The two-option orientation flow and immediate rotation behavior are suitable. Canonical copy should be defined in EN first. Keep manual orientation as the baseline; do not imply sensor-based auto-rotate unless verified in hardware. |
| `ENKU_first_start_controls_v1` | **Needs revision** | The information hierarchy is good, but the board still uses abstract command icons and explicitly says physical-button labels are TBD. Replace these with the final Waveshare control mapping after hardware input behavior is verified. |
| `ENKU_language_v1` | **Needs revision** | Settings flow is valid, but the language list must support the full first-wave set and use EN canonical strings. Keep interface language independent from book content/language. Verify scrolling/focus behavior with 7+ languages. |
| `ENKU_portrait_orientation_v2` | **Reference only** | This is a portrait layout/system reference rather than a single product screen. Keep it as a layout reference for headers, reading view and orientation settings; do not implement it as one screen. |

### Decisions from this block

1. **English is the canonical UI/source language.** Existing Russian boards are design references until corresponding EN boards are approved.
2. The first localization wave is **EN, RU, PL, DE, FR, ES, IT**.
3. Language selection must be implemented as a scrollable/focusable list rather than a fixed two-language layout.
4. First-start orientation remains a manual choice.
5. Hardware-control help cannot be finalized until the exact onboard control mapping is tested on the real Waveshare module.
6. First-start progress remains a three-step flow: Language → Orientation → Controls.


## Block 2 — Library

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_library_v1` | **Needs revision** | Grid structure works well in both orientations, including 4-column landscape and 2×2 portrait. Before implementation, define the actual book-focus state for physical navigation; the current board mainly demonstrates content/state rather than focus. Canonical copy and labels must be EN-first. Preserve visible-book position, filter, sort and grid/list mode across orientation changes. |
| `ENKU_library_list_v1` | **Needs revision** | List layout is strong and more information-dense than the grid. Keep cover thumbnail / title+author / state+progress structure. Add an explicit focused-row state for physical controls and define focus order across view/filter/sort controls. Grid/List selection persistence is approved. |
| `ENKU_library_search_v1` | **Needs revision** | Search behavior is valid, but the embedded keyboard is currently RU/EN-specific. EN is now the source UI language, while book titles may use other scripts. Keyboard layout/language selection therefore needs its own specification independent from interface language. Define focus traversal between search field, results and keyboard for physical controls. |
| `ENKU_filter_sort_v1` | **Approved** | Filter and sort model is clear: reading-state filter plus independent sort key/direction, persisted across list/grid and orientation. Implement canonical EN terminology first. On real hardware, selected vs focused radio rows must remain visually distinct. |
| `ENKU_empty_library_v1` | **Approved** | Clear empty-state hierarchy and a single useful CTA. Keep “Add books” as the initial focus. Do not use this state to mask missing/unavailable storage. |
| `ENKU_empty_filter_v1` | **Approved** | Correctly distinguishes “library has books” from “current filter returns none”. Active filter remains visible and the CTA resets only the filter while preserving view/sort state. |
| `ENKU_no_search_results_v1` | **Approved** | Good recovery state: query is preserved and the primary action reopens editing. Returning to Library should restore the prior library position. Empty query should show the full library rather than this error state. |
| `ENKU_book_details_v1` | **Needs revision** | Core information architecture is useful, but landscape and portrait currently expose different secondary content (e.g. description). Define one semantic content model for both orientations, with scrolling where needed. Specify fallbacks for missing cover/author/description/language and state-specific primary CTA: Start / Continue / Read again. |
| `ENKU_delete_book_v2` | **Approved** | Destructive confirmation is well structured. Default focus on Cancel is correct. Delete removes the local file plus ENKU progress/bookmarks; failure must preserve the library record for retry/recovery. |
| `ENKU_restart_reading_v1` | **Approved** | Confirmation model is correct. Reset reading position/progress only after confirmation while preserving the book file, bookmarks and per-book settings. A previously finished book returns to Reading state after restart. |

### Decisions from this block

1. **Grid and List are both first-class Library views** and the selected view is persisted.
2. Library state to preserve across orientation changes: **focused/visible book, view mode, filter and sort**.
3. Every Library board intended for implementation must define a visible **focus state for physical-button navigation**; selection/state and focus are separate concepts.
4. Search covers **title and author across the whole library**; active Library filters do not constrain search.
5. The on-screen keyboard is a separate subsystem from UI localization. Interface language must not force the keyboard layout.
6. Empty Library, empty Filter and no Search Results are three distinct states and must not be collapsed into one generic empty screen.
7. Book Details uses one semantic information model in both orientations; orientation changes layout, not available data.
8. Destructive/reset actions default focus to **Cancel**.
