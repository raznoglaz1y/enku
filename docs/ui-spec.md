# ENKU UI specification v1.0

This document is the canonical implementation baseline for the ENKU interface.

English (EN) is the source/reference language. All other languages are translations of the EN string set.

## 1. Interaction model

- ENKU is a non-touch interface.
- Navigation moves **focus**.
- Confirmation activates or changes the focused control.
- Back returns to the previous level or closes the current overlay/dialog.
- Focus, selected value, active tab and persistent state are different concepts and must never be represented by the same visual treatment.
- Returning from a child screen restores the parent's logical focus and scroll position.
- Orientation changes preserve logical focus and semantic content position, not raw pixel coordinates.

## 2. Core dimensions

Baseline values from the approved component boards:

- Primary button / input field height: **48 px**
- Corner radius: **4 px**
- Normal border: **1 px**
- Focus border: **2 px**
- Typical screen title: **20 px bold**
- Primary body text: **16 px**
- Supporting / metadata text: **12 px**
- Header back icon: **20 px**
- Header icon → title gap: **12 px**
- Row icon: **24 px**
- Dialog icon: **24 px**
- Dialog icon → text gap: **8 px**

These are design baselines, not immutable firmware constants. Any change must be validated in both 800×480 and 480×800 layouts.

## 3. Focus and selection states

### Focus
A focused row/control receives a **2 px outline**.

Focus means: this is the element that will respond to Confirm.

### Selected / applied value
A selected persistent value uses a check/radio/value indicator independent from focus.

A selected row can be unfocused.
A focused row can be unselected.

### Primary action in focus
A focused primary action uses black fill with white text.

### Active tab
The active tab uses black fill with white text.
If another tab has focus, that tab additionally receives the focus outline while the active tab remains visually active.

### Destructive and reset dialogs
Default focus is always the safe action:
- Cancel
- Continue
- Keep current

Never default focus to Delete / Replace / Reset / Power Off.

## 4. Headers

### Root header
Used for top-level sections such as Library.

Contains:
- section title;
- optional status bar above;
- optional trailing actions.

### Back header
Contains:
- back arrow;
- current screen title;
- optional trailing action/menu.

### Reading
Reading content is chrome-free by default.
Status/header controls appear only when the reading overlay/menu is invoked.

## 5. Lists and rows

- Entire row is focusable unless the row contains explicitly separate controls.
- Primary label may use up to **2 lines**.
- Supporting metadata uses **1 line** and ellipsis.
- Long content never reduces the font size to fit.
- Trailing icon/action is vertically centered against the whole row block.
- List state and current focus must remain distinguishable.
- Scroll restores the focused/visible item when returning or changing orientation.

## 6. Buttons

- Minimum baseline height: 48 px.
- Button text is optically centered.
- Standard horizontal screen/content inset: **24 px** on both sides.
- On single-column screens with one primary call-to-action, the primary CTA spans the **full content width between the 24 px insets** in both portrait and landscape.
- Examples: Continue, Finish setup, Connect, Save, Add books, Refresh, Done.
- The same full-width rule is used consistently across orientations; do not size a primary CTA to its label.
- One primary action per compact state/dialog where possible.
- Secondary actions use outline style.
- Multi-action dialogs, segmented controls, toolbars and genuine multi-column layouts follow their local grid instead of forcing each action full-width.

## 6.1 Icons

- ENKU uses the **approved Phosphor icon library** supplied in the original `icons(1).zip` asset set.
- Use the original SVG asset geometry directly. **Do not redraw, reinterpret, normalize or create replacement versions of existing icons.**
- Do **not** use Unicode characters, emoji, text glyphs or font symbols as production icon substitutes.
- Core icon size: **24 px**; header/back icon: **20 px**, unless a specific approved component defines otherwise.
- Scale the source SVG uniformly from its native `256 × 256` viewBox. Do not alter internal geometry to make icons appear artificially centered.
- Optical placement is handled by the surrounding component/layout, not by editing the source icon.
- Focus belongs to the control/container and never changes the underlying icon asset.
- If a required icon already exists in the approved Phosphor asset set, that exact file must be used.
- A new icon asset is added only when the approved set genuinely lacks the required concept; it must be reviewed before use.
- Existing approved/reference boards remain the visual source of truth for icon sizing and spacing.

## 7. Input fields and keyboard

- Input field height baseline: 48 px.
- Text-entry keyboard is a shared subsystem used by:
  - Wi-Fi password;
  - Library search;
  - in-book search;
  - future metadata editing where needed.
- Keyboard layout/input mode is independent from UI language.
- Initial supported text input modes: Latin, Cyrillic, numeric/symbol.
- Shift behavior:
  - one activation → next character uppercase;
  - second activation → Caps Lock;
  - third activation → lowercase.
- Password fields mask text by default and may expose an explicit Show password option.
- Credentials are persisted only after successful Wi-Fi connection.

## 8. Dialogs and overlays

- Modal/overlay height follows content.
- Background content remains visually present where practical, but does not receive focus.
- Back closes the overlay and restores parent focus.
- Dialog actions follow safe-default rules.
- Overlays must not silently change reading position.

## 9. Portrait / landscape rules

Same data model and same actions in both orientations.

Orientation changes layout, not functionality.

- Portrait generally uses one column.
- Landscape may use two columns where traversal is deterministic.
- Never hide important secondary information solely because orientation changes.
- Long lists remain scrollable.
- Current focus/selected item is preserved logically.

## 10. Long text and localization

English is canonical.

Initial translation set:
- EN
- RU
- PL
- DE
- FR
- ES
- IT

Rules:
- titles: up to two lines, then ellipsis;
- supporting metadata: one line, then ellipsis;
- do not shrink typography to fit translations;
- long SSIDs/book titles/chapter titles use the same truncation rules;
- every revised EN board must be stress-tested with long DE/FR/PL strings before being considered final.

## 11. Library

- Grid and List are both first-class views.
- Persist:
  - view mode;
  - filter;
  - sort;
  - focused/visible book.
- Search is global across title + author and is not constrained by the active Library filter.
- Empty Library, Empty Filter and No Search Results are distinct states.
- Book Details exposes the same semantic data in both orientations.

## 12. Reading

- Content-first reading view; chrome hidden by default.
- Five canonical presets:
  - Spacious
  - Comfortable
  - Standard
  - Compact
  - Dense
- Manual typography changes switch to **Custom**.
- Settings may be global or per-book.
- Typography/orientation changes preserve semantic text position before repagination.
- Contents and Bookmarks share one overlay.
- In-book search preserves the original reading position until the user opens a result.
- Search highlight must remain visible on monochrome/limited e-paper output.

## 13. Storage and import

- microSD/local files are part of MVP.
- Import validates before committing a Library record.
- Raw book file is logically separate from reading progress/bookmarks/per-book settings.
- Replacement is transactional.
- Incomplete imports use temporary files and are cleaned up on failure/cancel.
- No auto-format.
- No automatic deletion of books to make space.
- USB transfer is not committed until transport architecture is chosen.

## 14. Wi-Fi and local web management

Wi-Fi is optional for reading.

- Saved network: credentials are known.
- Trusted/preferred network: eligible for deterministic auto-connect priority.
- Avoid the ambiguous “Home network” concept.
- Local web management is session-scoped by default.
- Web UI scope includes:
  - multi-file upload / drag & drop;
  - book/file list;
  - safe delete and replace;
  - storage/free-space view;
  - import status/errors;
  - metadata inspection/correction;
  - cover preview/replacement/restoration;
  - mark read/unread;
  - reset progress;
  - clear per-book overrides.
- Full typography tuning stays on-device in the first version.

## 15. Sleep and power

- Sleep screens are static.
- No live clock, animations or periodic refresh.
- Book-cover sleep screen preserves source aspect ratio.
- Fall back to simple sleep screen when cover is missing/unreadable.
- Auto-sleep is suspended during imports/transfers/writes.
- Power-off semantics and wake behavior are not final until verified on real Waveshare hardware.
- Battery thresholds/percentage are not final until characterized with the actual LP505060 pack.

## 16. E-paper refresh principles

User-facing refresh options are not final until measured on the real panel.

General rules:
- avoid animation-style continuous updates;
- throttle transfer progress redraws;
- prefer partial refresh only where reliable;
- periodically use full refresh as required by real-panel ghosting measurements;
- never expose a “technical” setting that cannot be explained in user terms.

## 17. Canonical reference boards

System/reference material:
- `ENKU_ui_kit_core_v1`
- `ENKU_focus_states_v1`
- `ENKU_navigation_status_v2`
- `ENKU_portrait_orientation_v2`
- `ENKU_long_text_check_v1`

These are not production screens. They define reusable layout/interaction rules.


## 18. Design board format

- Canonical UI review boards are **PNG images**, matching the original RU design workflow.
- Each canonical board presents both **800 × 480 landscape** and **480 × 800 portrait** variants where applicable.
- SVG icon files in `design/icons/` remain the approved icon source/reference library.
- Screen behavior, dimensions, focus rules and implementation constraints live in this document and related docs; the PNG board is the visual reference.
- Generated EN SVG screen boards created during the remediation phase are considered **draft/intermediate artifacts**, not final canonical design boards.
- As each EN screen is visually finalized, replace its draft SVG reference with a PNG board and update the screen index / revision plan.
- Do not rely on GitHub SVG composition, external SVG references or hand-authored inline icon geometry for final review boards.
