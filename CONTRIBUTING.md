# Contributing to ENKU

ENKU is in an early product-definition and hardware-validation stage. Contributions are welcome, but they should stay aligned with the current reference platform and documented interaction model.

## Before contributing

Please read:

- [README.md](README.md)
- [ROADMAP.md](ROADMAP.md)
- [docs/ui-spec.md](docs/ui-spec.md)
- [docs/ui-audit.md](docs/ui-audit.md)

Open an issue before making a large behavioral, architectural or hardware change.

## Product principles

Contributions should preserve these project rules:

- ENKU is a focused e-reader, not a general-purpose tablet.
- The UI is non-touch and designed for physical controls.
- Focus, selected value and active state are separate concepts.
- English is the canonical/source UI language.
- Portrait and landscape are both first-class layouts.
- Reading works offline; Wi-Fi is an optional management layer.
- Hardware-dependent behavior must stay provisional until verified on the real board.
- E-paper limitations are design constraints, not edge cases.

## UI contributions

For UI changes:

- reference the affected screen/component;
- provide the canonical English copy first;
- include both 800 × 480 landscape and 480 × 800 portrait where applicable;
- preserve physical-control navigation;
- show focus explicitly;
- keep selected/active state separate from focus;
- stress-test long PL/DE/FR strings;
- do not replace approved icon assets with Unicode/emoji/font-symbol substitutes.

Canonical visual review boards are PNG. SVG screen compositions may be used as intermediate design artifacts only.

## Firmware contributions

Firmware is not yet implemented as a production reader in this repository.

When firmware work begins:

- keep hardware abstraction separate from application/reader logic;
- add build and flash instructions;
- document the exact board revision used;
- include validation results for display refresh, storage, power or memory-sensitive changes;
- avoid presenting unmeasured battery/runtime/refresh behavior as fact;
- keep changes focused and testable.

## Hardware / enclosure contributions

Do not publish guessed mechanical dimensions as authoritative.

Mechanical work should be based on measured hardware and should document:

- board/display dimensions;
- connector and control clearances;
- battery envelope;
- fasteners;
- print tolerances;
- assembly sequence.

## Pull requests

A good pull request should include:

1. the problem being solved;
2. the affected subsystem/screen;
3. what changed;
4. how it was validated;
5. any remaining hardware-dependent assumptions.

Prefer small, reviewable changes over large mixed refactors.

## Assets and licenses

Do not commit:

- credentials;
- private network information;
- copyrighted book content without permission;
- unlicensed third-party asset bundles.

The project license is not final yet. See [NOTICE.md](NOTICE.md).
