# ENKU UI revision plan

English is the canonical source language.

The goal of this phase is to convert every board marked **Needs revision** in the audit into an implementation-ready EN board built on `docs/ui-spec.md`.

## Workflow for every revised screen

1. Normalize behavior against `docs/ui-spec.md`.
2. Write canonical EN copy.
3. Produce the revised EN board in both landscape and portrait.
4. Verify focus vs selected/active states.
5. Stress-test with long DE/FR/PL text.
6. Update `design/screens/index.json` and `docs/screens.md`.
7. Mark the old RU board as historical/reference until localized replacements are available.

## Revision order

### First Start / Language
- [x] `ENKU_first_start_language_v1` → `design/screens/en/ENKU_first_start_language_v2.svg`
- [x] `ENKU_first_start_controls_v1` → `design/screens/en/ENKU_first_start_controls_v2.svg`
- [x] `ENKU_language_v1` → `design/screens/en/ENKU_interface_language_v2.svg`

### Library
- [x] `ENKU_library_v1` → `design/screens/en/ENKU_library_grid_v2.svg`
- [x] `ENKU_library_list_v1` → `design/screens/en/ENKU_library_list_v2.svg`
- [x] `ENKU_library_search_v1` → `design/screens/en/ENKU_library_search_v2.svg`
- [x] `ENKU_book_details_v1` → `design/screens/en/ENKU_book_details_v2.svg`

### Reading
- [x] `ENKU_quick_aa_preview_v3` → `design/screens/en/ENKU_quick_aa_preview_v4.svg`
- [x] `ENKU_typography_v1` → `design/screens/en/ENKU_typography_v2.svg`
- [x] `ENKU_search_in_book_v1` → `design/screens/en/ENKU_search_in_book_v2.svg`

### Settings / System
- [ ] `ENKU_settings_v1`
- [ ] `ENKU_display_settings_v1`
- [ ] `ENKU_sleep_settings_v1`
- [ ] `ENKU_about_device_v1`
- [ ] `ENKU_power_off_v1`
- [ ] `ENKU_low_battery_v1`

### Storage / Import
- [ ] `ENKU_storage_import_v1`
- [ ] `ENKU_import_methods_v1`
- [ ] `ENKU_replace_book_v1`
- [ ] `ENKU_transfer_progress_v2`
- [ ] `ENKU_transfer_interrupted_v1`
- [ ] `ENKU_not_enough_space_v1`

### Wi-Fi
- [ ] `ENKU_wifi_v1`
- [ ] `ENKU_wifi_password_v3`
- [ ] `ENKU_saved_networks_v1`
- [ ] `ENKU_empty_networks_v1`
- [ ] `ENKU_wifi_transfer_v1`

## Definition of done

A revised board is complete only when:
- EN copy is canonical;
- both orientations are represented;
- focus behavior is explicit;
- semantic content is equivalent in both orientations;
- no unverified hardware behavior is presented as final;
- long-text/localization constraints are checked;
- linked documentation/index entries are updated.
