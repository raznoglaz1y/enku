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


## Block 3 — Reading

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_reading_menu_v2` | **Approved** | Menu structure is coherent and compact for physical navigation. Keep Font, Contents & Bookmarks, Add Bookmark, Search in book, Orientation, About book and Sleep. Canonical labels should be EN-first. Current preset/orientation values may remain as secondary text. |
| `ENKU_quick_aa_preview_v3` | **Needs revision** | The five presets and live-preview concept are strong, but focus vs selected preset must be explicit for hardware controls. Preset values are provisional until tested on the real panel. Keep “All settings” as the path to full Typography. |
| `ENKU_typography_v1` | **Needs revision** | The model is correct: five presets, Custom, per-book override, size/line spacing/margins/alignment. Before implementation, define min/max/step values and navigation behavior for +/- and alignment controls. Manual changes switch preset to Custom. |
| `ENKU_contents_bookmarks_v3` | **Approved** | Contents/Bookmarks as a compact overlay is a good fit. Preserve reading position under the overlay. Current chapter/bookmark focus model is suitable; long lists scroll. Bookmark actions need a separate interaction spec if edit/delete is supported. |
| `ENKU_empty_bookmarks_v1` | **Approved** | Correct empty state inside the combined overlay. Contents remains reachable. Returning closes overlay without changing reading position. |
| `ENKU_no_contents_v1` | **Approved** | Correctly treats missing ToC as non-fatal. Bookmarks remain available. Do not infer chapters from arbitrary headings unless the parser has explicit structure. |
| `ENKU_search_in_book_v1` | **Needs revision** | Result-list model is valid. Search must reuse the shared keyboard subsystem and preserve original reading position until a result is explicitly opened. Define pagination/batching strategy for many matches to avoid expensive full-book rendering/search UI updates. |
| `ENKU_search_match_v1` | **Approved** | Match-navigation concept is clear: previous/next match, counter, contextual text, return to result list. Search highlight must be compatible with monochrome e-paper (weight/underline/inversion rather than relying on gray only). |
| `ENKU_book_finished_v1` | **Approved** | Completion state is clear. Reaching beyond the last page sets progress to 100% and marks the book Finished/Read. Default focus on “Back to Library” is appropriate; Back keeps the reader on the final page. |
| `ENKU_navigation_status_v2` | **Reference only** | This is a system reference board for headers, reading chrome and status icons rather than a production screen. Use it to derive shared components and status-bar rules. |

### Decisions from this block

1. Reading remains a **content-first screen** with chrome hidden by default.
2. The Reading Menu is the primary gateway to reader actions; Quick Aa is a shortcut, not a second settings system.
3. The five canonical reading presets remain **Spacious, Comfortable, Standard, Compact, Dense**; manual edits produce **Custom**.
4. Typography can be global or per-book. Per-book settings override global reading settings only for that title.
5. Changing typography or orientation must preserve the semantic reading position, then repaginate around that position.
6. Contents and Bookmarks share one overlay and must not disturb the underlying reading position.
7. Missing ToC and empty Bookmarks are valid independent states.
8. In-book search must preserve the pre-search reading position until the user opens a result.
9. Search highlighting cannot depend on grayscale alone; it must survive monochrome/limited e-paper rendering.
10. Shared Reading chrome/status elements should be implemented from a reusable component spec, using `ENKU_navigation_status_v2` as reference.


## Block 4 — Settings / System

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_settings_v1` | **Needs revision** | The overall information architecture is good, but the Settings root should be normalized around the final feature set and EN-first terminology. Keep Reading, Display, Wi-Fi, Language, Storage, Sleep, About and Power Off. Focus order in landscape must be explicitly defined across two columns. |
| `ENKU_display_settings_v1` | **Needs revision** | Orientation control is valid. The e-paper refresh options are still conceptual: “Balanced”, “Clean”, full-refresh cadence and manual refresh must be validated against the real Waveshare panel/driver behavior before they become user-facing settings. Do not expose implementation knobs that cannot be measured or explained. |
| `ENKU_sleep_settings_v1` | **Needs revision** | Sleep timer + sleep-screen choice is a good model, but actual timeout values and wake behavior must be verified on hardware. Automatic sleep should pause during active transfers/imports. Manual Sleep remains available from the Reading Menu. |
| `ENKU_sleep_cover_v1` | **Approved** | A static full-screen book cover is an appropriate e-paper sleep screen. Preserve aspect ratio; do not stretch/crop. If no usable cover exists, fall back to the simple sleep screen. No clock, Wi-Fi status or animated content. |
| `ENKU_sleep_screen_v1` | **Approved** | Good minimal fallback sleep state. Static title/author/progress is suitable and avoids unnecessary refreshes. If no book is open, fall back to ENKU + sleep indicator only. |
| `ENKU_about_device_v1` | **Needs revision** | Keep it informational and read-only, but populate values from real firmware/hardware constants. Use the exact Waveshare board name, firmware version and display resolution. Add build/version metadata only if it is useful for issue reports and reproducibility. |
| `ENKU_power_off_v1` | **Needs revision** | Confirmation UX is good and default focus on Cancel is correct, but true power-off / wake behavior depends on the real Waveshare board and power architecture. Do not promise a behavior until hardware testing confirms what “Power off” can actually do. |
| `ENKU_low_battery_v1` | **Needs revision** | Warning UX is useful, but threshold percentage and battery accuracy are unverified. The real board’s battery measurement capability, calibration and safe critical-voltage behavior must be measured with the LP505060 pack before exposing exact percentages. |
| `ENKU_focus_states_v1` | **Reference only** | This is a system interaction reference, not a product screen. Keep it as the canonical visual rule for focused row, selected value, focused action and active tab. Selection and focus remain independent. |

### Decisions from this block

1. Settings root categories are: **Reading, Display, Wi-Fi, Language, Storage, Sleep, About, Power Off**.
2. Landscape Settings uses two columns only if focus traversal remains deterministic and easy to explain.
3. Display refresh policy is an **implementation-driven setting**: expose only options validated on the real Waveshare e-paper hardware.
4. Sleep screens are strictly static. No live clock, animated elements or periodic status refresh.
5. Sleep cover preserves source aspect ratio and falls back to the simple sleep screen when unavailable or unreadable.
6. Auto-sleep is suspended during active import/transfer/write operations.
7. About values come from firmware/hardware constants, not hard-coded mock strings.
8. “Power off” semantics are deferred until the real board’s power and wake behavior are tested.
9. Battery percentage/thresholds are deferred until real LP505060 + board measurement behavior is characterized.
10. `ENKU_focus_states_v1` becomes the shared focus/selection visual reference for the whole UI.


## Block 5 — Storage / Import / Transfer

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_storage_import_v1` | **Needs revision** | Storage overview is useful, but labels currently imply USB/Wi-Fi as settled import methods. For MVP, microSD/file access is confirmed; Wi-Fi is planned; USB remains implementation-dependent. Capacity/free-space values must come from the real filesystem. “Refresh library” should rescan without duplicating books or destroying reading data. |
| `ENKU_import_methods_v1` | **Needs revision** | The selection model is clear, but USB must not be presented as guaranteed until the transport mode is chosen and implemented. Keep Wi-Fi as planned and microSD/files as the always-available local path. |
| `ENKU_file_browser_v1` | **Approved** | Strong fit for physical navigation. Folders first, then files; back goes up one level; file state can indicate “already in library”. Preserve current directory and focused row across orientation changes. Adding a book must not remove the source file from the card. |
| `ENKU_book_validation_v1` | **Approved** | Validation-before-add is correct. Validate structure/openability locally before committing a new library record. Cancel removes only the temporary incoming file. Existing books and reading data stay untouched until a replacement succeeds. |
| `ENKU_replace_book_v1` | **Needs revision** | Confirmation flow is good, but duplicate detection cannot rely on filename alone. Define book identity strategy (metadata/hash/normalized identifier). Preserve progress/bookmarks/settings only when the new file is confirmed to represent the same book or when the user explicitly chooses replacement. |
| `ENKU_import_summary_v1` | **Approved** | Session summary is useful for batch operations. Counts should be mutually exclusive: added / replaced / skipped / failed. Failed items need concise reasons and a scrollable list when necessary. |
| `ENKU_import_complete_v1` | **Approved** | Good single-book success state. “Open” starts/resumes the added book; “Back to files” restores folder and focus. Newly added books enter Library as New unless prior reading state is intentionally restored. |
| `ENKU_transfer_progress_v2` | **Needs revision** | Progress UI is valid for network transfer, but e-paper should update progress at a throttled cadence rather than every byte/percent change. Exact update interval should be measured on hardware. Preserve completed books if a later file fails. |
| `ENKU_cancel_transfer_v1` | **Approved** | Correct confirmation model with Continue as safe default. Cancel removes only the incomplete temporary file and keeps previously completed imports/results. |
| `ENKU_transfer_interrupted_v1` | **Needs revision** | Recovery concept is sound, but behavior depends on transport. For first implementation, Wi-Fi can restart a failed upload from the beginning unless resumable transfer is explicitly implemented. Completed books remain committed. |
| `ENKU_no_memory_card_v1` | **Approved** | Distinct from empty library and read error. Retry detection is the primary action. Existing ENKU metadata/progress should not be deleted just because the card is missing. |
| `ENKU_memory_card_error_v1` | **Approved** | Correctly separates “card present but unreadable” from “no card”. Retry should remount/reopen; never offer implicit formatting or destructive recovery. |
| `ENKU_not_enough_space_v1` | **Needs revision** | The state is valid, but required/free-space calculation must include temporary import overhead and filesystem constraints. Storage management destination should be clearly defined; ENKU must never auto-delete books to make room. |
| `ENKU_usb_transfer_v2` | **Reference only** | Keep as a concept/reference board only. USB transfer mode, host/device behavior and file-exposure strategy are not yet selected and should not be promised in MVP documentation. |

### Decisions from this block

1. **microSD / local files are part of the first usable MVP path.**
2. Wi-Fi import remains planned; **USB transfer is not committed** until the transport architecture is chosen and verified.
3. Library metadata, progress, bookmarks and per-book settings are logically separate from the raw book file.
4. Import always validates a book before creating/replacing a committed library record.
5. Duplicate detection requires an identity strategy stronger than filename equality.
6. Replacing a file must be transactional: keep the old usable book until the new one is validated and safely written.
7. Transfer/import operations may create temporary files; incomplete temporary files are cleaned up on cancel/failure.
8. Already completed books in a multi-file session remain committed even if a later file fails.
9. E-paper progress UI is **throttled**, not continuously animated.
10. Missing card, unreadable card and insufficient space remain separate system states.
11. ENKU never auto-formats storage and never deletes existing books automatically to make space.


## Block 6 — Wi-Fi / Networking

| Board | Status | Audit notes |
| --- | --- | --- |
| `ENKU_wifi_v1` | **Needs revision** | Overall structure is good: Wi-Fi on/off, available networks, current connection and rescan. However, scan/update behavior, connection timeout and signal-state wording must be tied to real ESP32-S3 behavior. Focus order in landscape must remain deterministic across the network list and current-connection actions. |
| `ENKU_wifi_password_v3` | **Needs revision** | Password-entry flow is valid and should reuse the shared on-screen keyboard subsystem. EN is the source UI language, but password input must support symbols/numbers independently of interface language. “Show password” is acceptable; credentials must only be persisted after successful connection. |
| `ENKU_saved_networks_v1` | **Needs revision** | Saved-network management is useful, but the current “Home network” concept should be simplified into an explicit **trusted / preferred network** model. Define auto-connect priority deterministically and avoid ambiguous hidden ranking rules. |
| `ENKU_empty_networks_v1` | **Needs revision** | Empty state is fine visually, but copy currently mixes “saved” and “trusted/home” concepts. Rework after the trusted/preferred-network model is finalized. Primary action should return to Wi-Fi scan/selection. |
| `ENKU_forget_network_v1` | **Approved** | Confirmation model is correct with Cancel as default focus. Forgetting removes stored credentials and auto-connect/trusted status for that network. If currently connected, disconnect first and return to the saved-networks list. |
| `ENKU_wifi_transfer_v1` | **Needs revision** | Local-browser transfer is a good planned workflow. The device should expose a temporary local web endpoint while on the same LAN. Exact URL/hostname strategy, service lifetime and authentication/security model need implementation decisions before the screen is final. |

### Decisions from this block

1. Wi-Fi is **optional for reading**; the core reader and microSD workflow must remain fully usable offline.
2. Credentials are saved only after a successful connection.
3. The on-screen keyboard is shared across Wi-Fi passwords, search and other text fields; keyboard layout/input mode is independent from UI language.
4. Saved networks and trusted/preferred networks are distinct concepts: saved = credentials known; trusted/preferred = eligible for automatic connection priority.
5. Auto-connect behavior must be deterministic and documented. Do not hide opaque ranking logic from the user.
6. “Home network” terminology is dropped in favor of **trusted/preferred network** terminology.
7. Local Wi-Fi transfer runs only while the transfer screen/session is active, unless implementation later proves a safe reason to keep the service alive longer.
8. Transfer URL/hostname and any authentication mechanism must be decided from the real firmware architecture, not hard-coded from mockups.
9. Active transfer suppresses auto-sleep and must survive ordinary UI focus/navigation without silently aborting.
10. Network failures should distinguish at least: authentication failure, connection timeout/unreachable network, and transfer interruption.
