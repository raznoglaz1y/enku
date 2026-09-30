# ENKU project plan

This document defines the current high-level implementation order for ENKU.

## Phase 0 — Design baseline
- [#1 UI audit](https://github.com/raznoglaz1y/enku/issues/1)
- [#2 Localization: EN / PL / DE / FR / ES / IT](https://github.com/raznoglaz1y/enku/issues/2)
- [#11 ENKU wordmark](https://github.com/raznoglaz1y/enku/issues/11)

### Localization strategy

Russian remains the reference UI language. The first localization wave targets English, Polish, German, French, Spanish and Italian.

Localization must be implemented as a firmware-level string system, not as duplicated hard-coded screens. UI layouts must tolerate longer translations, and all language packs must preserve the same physical-button navigation and focus behavior.

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
- [#9 Wi-Fi setup and local transfer](https://github.com/raznoglaz1y/enku/issues/9)

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
