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
