# ENKU project plan

This document defines the current high-level implementation order for ENKU.

## Phase 0 — Design baseline
- [#1 UI audit](https://github.com/raznoglaz1y/enku/issues/1)
- [#2 Localization: EN / PL / DE / FR / ES / IT](https://github.com/raznoglaz1y/enku/issues/2)
- [#11 ENKU wordmark](https://github.com/raznoglaz1y/enku/issues/11)

### Localization strategy

English is the source/reference UI language. The first localization wave targets Russian, Polish, German, French, Spanish and Italian.

Localization must be implemented as a firmware-level string system, not as duplicated hard-coded screens. Canonical UI copy, terminology and localization keys are defined in English first; all other languages map to that source. UI layouts must tolerate longer translations, and all language packs must preserve the same physical-button navigation and focus behavior.

The first firmware releases should prioritize Latin and Cyrillic scripts. Larger CJK font sets can be evaluated separately once storage and RAM costs are measured on real hardware.

## Phase 1 — Hardware bring-up
- [#3 Firmware foundation](https://github.com/raznoglaz1y/enku/issues/3)
- [#4 Display rendering and refresh strategy](https://github.com/raznoglaz1y/enku/issues/4)
- [#5 Physical-button navigation](https://github.com/raznoglaz1y/enku/issues/5)

## Phase 2 — Reading MVP
- [#7 Book format and parsing strategy](https://github.com/raznoglaz1y/enku/issues/7)
- [#6 Library and reader core](https://github.com/raznoglaz1y/enku/issues/6)
- [#8 Storage and book import](https://github.com/raznoglaz1y/enku/issues/8)

## Phase 3 — Connected features
- [#9 Wi-Fi setup, local transfer and web management UI](https://github.com/raznoglaz1y/enku/issues/9)

### Local web management

ENKU should expose a lightweight local web interface while connected to the same LAN as the user's phone or computer.

First-version scope:
- drag-and-drop single/multi-book upload;
- file/book listing;
- delete and safe replace;
- storage/free-space view;
- transfer/import progress and errors;
- basic metadata inspection/correction where useful;
- book-cover preview and manual replacement;
- restore the original embedded cover when available;
- limited book actions such as mark read/unread, reset progress and clear per-book reading overrides.

Full typography and reading configuration remains on-device in the first version, because those settings are best adjusted while immediately viewing the result on the e-paper screen.

The web UI is a management tool, not a second reader application.

## Phase 4 — Portable-device behavior
- [#10 Power management, sleep and battery behavior](https://github.com/raznoglaz1y/enku/issues/10)

## First hardware target

Waveshare ESP32-S3-ePaper-3.97 with its onboard controls and TF/microSD slot.

Battery target: LP505060 Li-Po, 3.7 V, 2000 mAh, PCM, with a suitable connector fitted for the Waveshare board.

## Alpha definition

The first useful hardware alpha should:
1. boot reliably;
2. render the ENKU UI on the real e-paper panel;
3. navigate entirely with physical controls;
4. discover and open at least one supported book format from microSD;
5. persist reading position;
6. provide basic typography controls;
7. enter and wake from a low-power state predictably.

The exploded hardware illustration is intentionally postponed until the mechanical layout is based on verified board geometry.
