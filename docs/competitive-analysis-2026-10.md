# ENKU competitive analysis — October 2026

Status: **product-strategy baseline / revisit before production pricing**

This document captures the market comparison used before freezing the custom ENKU mainboard and the BOM for each product variant.

## Product thesis

ENKU should not compete as "another cheap small e-reader".

Current compact readers already cover the low-cost hardware segment aggressively. ENKU instead targets the overlap of:

- compact pocket-reader ergonomics;
- physical-first controls;
- open hardware and open firmware;
- local/offline ownership of books and state;
- a coherent reader UI rather than a generic development board;
- motion/orientation controls;
- magnetic cover support;
- an optional dock with a dedicated low-refresh Dock Mode;
- optional frontlight and wireless charging without forcing those costs into every unit.

The closest product idea is therefore closer to:

> open-hardware maker platform + finished pocket-reader UX

than to a conventional Kindle/Kobo replacement.

## Competitor set

### XTEINK X4 Classic

Reference:
- 4.3-inch pocket reader;
- physical buttons;
- microSD;
- wireless connectivity;
- motion page turning;
- direct price around USD 79 at the time of this analysis.

Source:
https://www.xteink.com/products/xteink-x4-classic-pocket-ereader

Implication for ENKU:
- Base cannot rely on "small + buttons + microSD + motion" as unique features;
- Base pricing must remain disciplined;
- ENKU must differentiate through openness, software, USB-C/serviceability, local management and the Dock ecosystem.

### XTEINK X4 Pro

Reference:
- 4.3-inch;
- warm/cool frontlight;
- touch + physical controls;
- microSD;
- wireless connectivity;
- direct price around USD 99 at the time of this analysis.

Source:
https://www.xteink.com/products/xteink-x4-pro-pocket-ereader

Implication for ENKU:
- a frontlight alone is not enough to justify Pro;
- ENKU Pro needs the complete open-hardware / physical-first / Dock / motion story;
- Pro Wireless should remain a separate option rather than increasing every Pro BOM.

### BOOX Picco

Reference:
- 3.97-inch, 480 × 800 / 235 ppi;
- warm/cool frontlight;
- touch + physical controls;
- microSD;
- Wi-Fi / Bluetooth;
- mature BOOX reading software;
- EU price around EUR 109.99 at the time of this analysis.

Source:
https://euroshop.boox.com/pages/picco

Implication for ENKU:
- Picco is the closest finished commercial product by size;
- ENKU should not try to match the breadth of the BOOX software stack;
- ENKU should focus on coherent, local, offline-first workflows and an understandable open platform.

### Kobo Clara BW

Reference:
- 6-inch 300 ppi display;
- ComfortLight PRO;
- touch;
- waterproofing;
- mature Kobo store and reading ecosystem;
- Poland pricing around PLN 699 at the time of this analysis.

Source:
https://ereader.kobo.com/pl-pl/products/kobo-clara-bw

Implication for ENKU:
- Kobo is not the primary BOM competitor;
- bookstore/DRM and waterproofing are mainstream strengths that ENKU should not chase in R0.x;
- ENKU instead competes on ownership, openness, compactness and hardware extensibility.

### PocketBook Verse Pro

Reference:
- 6-inch 300 dpi;
- SMARTlight;
- physical controls + touch;
- broad format support;
- audio/TTS;
- IPX8.

Source:
https://pocketbook.de/en/e-reader

Implication for ENKU:
- PocketBook remains stronger in format breadth, dictionaries, audio/TTS and mature retail-reader functionality;
- ENKU should avoid audio feature creep in the first hardware generation.

### Inkplate 5 Gen2

Reference:
- open-source e-paper development platform;
- maker/OEM oriented;
- EU price around EUR 74.95 at the time of this analysis.

Source:
https://soldered.com/products/inkplate-5-gen2/

Implication for ENKU:
- Inkplate is the closest competitor philosophically;
- ENKU needs a substantially more finished reader product experience than a generic open e-paper board.

## ENKU software position

### Strong / differentiating targets

The planned ENKU stack includes:

- offline-first reading;
- EPUB / FB2 / TXT Reader v1;
- Grid and List Library views;
- New / Reading / Finished filters;
- Title / Author / Recently opened / Recently added sorting;
- global title/author search;
- stable book identity independent of filenames;
- bookmarks;
- table of contents;
- in-book search;
- semantic reading-position persistence;
- crash-aware A/B + generation + CRC persistence for critical state;
- global and per-book typography;
- five reading presets plus Custom;
- local browser-based multi-file upload;
- local metadata correction;
- cover replacement;
- storage management without a cloud account;
- physical-first UI with deterministic focus;
- motion page turning;
- automatic orientation and 180-degree handedness;
- magnetic-cover wake/sleep;
- configurable Dock Mode;
- open hardware / firmware / enclosure sources.

The most important software differentiators are:

1. **local web library management without account dependency**;
2. **physical-first deterministic interaction**;
3. **Dock Mode as a real product mode rather than a static clock screen**;
4. **open and reproducible persistence / hardware architecture**.

## Software gaps to acknowledge

ENKU Reader v1 intentionally does not yet match mature commercial readers in every area.

### PDF

PDF is deferred until the reflowable reader is stable.

This is a launch gap versus BOOX, Kobo and PocketBook.

### Dictionaries / translation

Offline dictionary support should be added after the core EPUB/FB2/TXT reader is stable and before PDF becomes a priority.

Target future flow:

```text
select / focus word
→ Dictionary
→ local dictionary lookup
→ no cloud account required
```

Cloud translation is not a prerequisite.

### Store / DRM ecosystem

Not a target.

ENKU is positioned as an open/local reader, not a Kindle/Kobo-style commercial storefront.

### Audio / TTS

Not planned for R0.x.

Avoiding audio keeps cost, thickness, power and firmware complexity under control.

### Waterproofing

Not planned for R0.x kits.

Sealing conflicts with serviceability, maker assembly and low-cost kit production.

## Display strategy

### Base

Primary candidate:
**Good Display GDEY0397T81P**

Target:
- 3.97-inch;
- 800 × 480;
- ~235 ppi;
- SSD1677;
- no frontlight;
- 24-pin 0.5 mm FPC.

Source:
https://www.good-display.com/product/613.html

### Pro / Pro Wireless

Primary candidate:
**Good Display GDEY0426T82-FL01C**

Target:
- 4.26-inch;
- 800 × 480;
- ~218 ppi;
- SSD1677;
- bonded frontlight;
- warm + cool white LEDs;
- 24-pin EPD FPC + separate frontlight FPC.

Source:
https://www.good-display.com/product/880.html

The older GDEQ0426T82-FL01C is treated as legacy/EOL and should not be designed into a new product.

Replacement notice:
https://www.good-display.com/product/1208.html

## Product variants and BOM ceilings

These are **design stop-limits**, not supplier quotes.

| Variant | Target landed BOM | Target Kickstarter | Target website |
| --- | ---: | ---: | ---: |
| Electronics Kit | <= PLN 155 | PLN 249–279 | PLN 299 |
| Base | <= PLN 180 | PLN 299–329 | PLN 349 |
| Cover | <= PLN 200 | PLN 349–379 | PLN 399 |
| Pro | <= PLN 230 | PLN 449–479 | PLN 499 |
| Pro Wireless | <= PLN 260 | PLN 479–519 | PLN 549 |
| Dock | <= PLN 40 | PLN 79–99 | PLN 119 |

If real supplier / assembly quotes miss these targets materially, ENKU should reduce or substitute BOM items **before final PCB routing**.

## Variant positioning

### Electronics Kit

For makers.

Competes more directly with Inkplate and other open e-paper platforms than with BOOX.

### Base

Compact open reader.

Needs to stay close enough to XTEINK X4 Classic pricing that openness and software can justify the remaining premium.

### Cover

Base plus magnetic folio and Hall wake/sleep.

This is a value-added mechanical tier rather than a separate electronics platform.

### Pro

4.26-inch warm/cool frontlight.

Pro should not be sold merely as "Base with light"; it should represent the premium reading configuration with the same open platform and Dock ecosystem.

### Pro Wireless

Pro plus Qi receiver / coil.

This is the highest hardware tier and should remain optional so Qi cost, thickness and thermal constraints do not burden Base or ordinary Pro.

### Dock

Separate passive accessory for charging and Dock Mode.

The first dock should not require its own MCU.

## Competitive product rule

Every feature added before BOM freeze must answer one of these questions:

1. Does it materially improve reading?
2. Does it materially improve ownership / openness / repairability?
3. Does it create a defensible ENKU ecosystem feature such as Dock Mode?
4. Can it be DNP / optional so Base cost stays protected?

If none apply, it should not be on the R0.x mainboard.

## Software priority after market review

Current priority:

1. robust EPUB / FB2 / TXT;
2. Library + persistence;
3. typography;
4. bookmarks / TOC / in-book search;
5. local web library manager;
6. Dock Mode;
7. offline dictionary;
8. PDF after the reflowable reader is solid.

Audio/TTS, DRM/store integration and waterproofing remain out of R0.x scope.

## Revalidation trigger

Re-run this comparison:
- before Kickstarter pricing;
- after first PCBWay/assembly quote;
- after first Good Display volume quote;
- before committing to a 100+ unit batch.
