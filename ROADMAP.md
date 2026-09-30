# Roadmap

## Design baseline
- [x] Collect latest available UI boards (59 boards, including diagnostic sheets).
- [x] Document core components, optical alignment and focus states.
- [x] Document principal navigation and return behavior.
- [x] Add Russian and English logical layer diagrams.
- [ ] Audit historical boards for touch wording and outdated alignment.
- [ ] Produce an English counterpart for every Russian screen.
- [ ] Verify each localized board in both orientations.
- [ ] Separate diagnostic boards from production screens in final inventory.

## Repository launch
- [x] English and Russian README, contribution guide and issue templates.
- [ ] Select code/design licenses and verify asset provenance.
- [ ] Connect GitHub; choose owner and repository name (suggested: enku).
- [ ] Publish first repository revision.
- [ ] Create milestones: Design baseline, Hardware UI prototype, Reader MVP, Import & connectivity.
- [ ] Convert remaining roadmap items into GitHub issues.

## Hardware UI prototype
- [ ] Confirm physical buttons, GPIO assignments and wake behavior.
- [ ] Bring up display with supported update modes.
- [ ] Validate Noto Sans Cyrillic/Latin and actual SVG rasterization.
- [ ] Implement shared layout and optical alignment helpers.
- [ ] Test orientation, focus and partial/full refresh on hardware.

## Reader MVP
- [ ] Decide supported book formats and parser.
- [ ] Implement library indexing and list/grid navigation.
- [ ] Implement text layout, five presets and Custom settings.
- [ ] Persist text position, progress, bookmarks and per-book settings.
- [ ] Add table of contents, in-book search and empty/error states.
- [ ] Add sleep and restore behavior.

## Import & connectivity
- [ ] Card import, validation, duplicates and import summaries.
- [ ] Saved credentials and explicit trust per network.
- [ ] Local Wi-Fi transfer, interruption and cancellation handling.
- [ ] Choose USB transport before implementing its screens.
- [ ] Define position/bookmark migration when replacing changed books.

## Release validation
- [ ] RU/EN × portrait/landscape checks.
- [ ] Long text, missing card, invalid book and interrupted transfer tests.
- [ ] Reboot/power loss recovery and persistent settings tests.
- [ ] Hardware documentation, reproducible build and first tagged release.
