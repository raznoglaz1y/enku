# ENKU competitive positioning, software differentiation and BOM targets

Status: **planning / October 2026**

This document compares ENKU with current pocket e-readers and adjacent open-hardware products before the custom mainboard and variant BOMs are frozen.

## Scope

Primary comparison set:

- XTEINK X4 Classic
- XTEINK X4 Pro
- BOOX Picco
- Kobo Clara BW
- PocketBook Verse Pro
- Inkplate 5 Gen2

ENKU is not intended to beat every mass-market reader on every feature. The product strategy is to combine:

- compact pocket format;
- physical-first interaction;
- strong local/offline software;
- open hardware and firmware;
- reproducible kits;
- motion/orientation controls;
- Dock Mode;
- optional frontlight and Qi without burdening the Base BOM.

## Competitive price reality

Current official / direct prices used as references:

- XTEINK X4 Classic — USD 79
- XTEINK X4 Pro — USD 99
- BOOX Picco EU — EUR 109.99
- Kobo Clara BW Poland — PLN 699
- PocketBook Verse Pro Germany — EUR 169
- Inkplate 5 Gen2 — EUR 74.95

Price sources:
- https://www.xteink.com/products/xteink-x4-classic-pocket-ereader
- https://www.xteink.com/products/xteink-x4-pro-pocket-ereader
- https://euroshop.boox.com/pages/picco
- https://ereader.kobo.com/pl-pl/products/kobo-clara-bw
- https://pocketbook.de/en/e-reader
- https://soldered.com/products/inkplate-5-gen2/

Important: direct USD prices may exclude some shipping/import/VAT effects for a Polish customer, so the model keeps them as direct-product references rather than pretending they are identical landed prices.

## ENKU target economics

Target landed/BOM ceilings before final supplier quotes:

| Variant | Target landed/BOM | Kickstarter Early Bird | Kickstarter standard | Website target |
| --- | ---: | ---: | ---: | ---: |
| Electronics Kit | <= PLN 155 | PLN 249 | PLN 279 | PLN 299 |
| Base | <= PLN 180 | PLN 299 | PLN 329 | PLN 349 |
| Cover | <= PLN 200 | PLN 349 | PLN 379 | PLN 399 |
| Pro | <= PLN 230 | PLN 449 | PLN 479 | PLN 499 |
| Pro Wireless | <= PLN 260 | PLN 479 | PLN 519 | PLN 549 |
| Dock | <= PLN 40 | PLN 79 | PLN 99 | PLN 119 |

These are internal cost/positioning targets, not announced retail prices.

## Display strategy

### Base

Primary candidate: **Good Display GDEY0397T81P**

- 3.97 inch
- 800 x 480
- 235 PPI
- SSD1677
- 24-pin 0.5 mm FPC
- 96.62 x 56.24 x 0.92 mm
- no frontlight

Source:
https://www.good-display.com/product/613.html

### Pro

Primary candidate: **Good Display GDEY0426T82-FL01C**

- 4.26 inch
- 800 x 480
- 218 PPI
- SSD1677
- bonded frontlight
- warm + cool LEDs
- 24-pin 0.5 mm EPD FPC + 6-pin frontlight FPC
- 105.33 x 62.37 x 1.8 mm

Source:
https://www.good-display.com/product/880.html

The older GDEQ0426T82-FL01C is EOL and Good Display directs customers to the GDEY replacement:
https://www.good-display.com/product/1208.html

## Hardware differentiation

### Against XTEINK X4 Classic

X4 Classic is a serious direct competitor because it is compact, cheap, has physical controls, microSD and motion page turning.

ENKU must therefore not rely on “small + buttons + gesture page turn” as its only story.

ENKU differentiation must come from:
- USB-C as standard rather than pogo-only charging;
- explicit hard power-off;
- open hardware files;
- richer local library management;
- 180-degree handedness / orientation model;
- Dock Mode;
- optional magnetic cover;
- optional Pro frontlight;
- optional Qi.

X4 Classic source:
https://www.xteink.com/products/xteink-x4-classic-pocket-ereader

### Against XTEINK X4 Pro

X4 Pro raises the minimum bar for Pro:
- 4.3 inch
- frontlight warm/cool
- touchscreen + buttons
- microSD
- 1100 mAh battery
- Wi-Fi / Bluetooth
- direct price USD 99

Therefore ENKU Pro cannot justify a premium using frontlight alone.

ENKU Pro must justify its higher price through:
- fully open hardware;
- repairable / reproducible kit design;
- USB-C + Dock;
- motion/orientation controls;
- Dock Mode dashboards;
- configurable physical-first UI;
- local web library management;
- optional Qi.

Source:
https://www.xteink.com/products/xteink-x4-pro-pocket-ereader

### Against BOOX Picco

Picco is the closest current finished-device competitor to ENKU Base/Pro because it has:
- 3.97 inch 480 x 800 / 235 PPI
- frontlight warm/cool
- touch + physical buttons
- microSD expandable to 2 TB
- Wi-Fi / BT
- broad document format support
- BOOX Tiles web/app transfer
- Pomodoro, Todo and Countdown apps

Source:
https://euroshop.boox.com/pages/picco

BOOX also has a much more mature reading stack, including TOC, bookmarks, dictionary/translation, reading-data sync and rich reader controls.

Examples:
- https://help.boox.com/hc/en-us/articles/10701496119060-TOC-Bookmarks
- https://help.boox.com/hc/en-us/articles/8569333726740-Translation-and-Dictionary
- https://help.boox.com/hc/en-us/articles/10701453841044-Reading-Data-Syncing

ENKU must not pretend to beat BOOX on software breadth at launch.

Instead ENKU should emphasize:
- simpler deterministic UI;
- no touch dependency;
- no account dependency;
- transparent local state;
- open implementation;
- Dock Mode;
- repairability / kit ownership.

## Software comparison

### Areas where ENKU can be stronger

#### Local-first library management

Planned ENKU browser manager:
- upload books locally;
- view storage;
- edit metadata;
- replace/remove covers;
- delete/replace files safely;
- reset progress/settings;
- no cloud account required.

This is a meaningful differentiator in the pocket-reader category if it stays fast and reliable.

#### Physical-first navigation

ENKU is designed around deterministic focus and physical actions from the start.

This is different from shrinking a touch-first interface onto a small display.

#### Local state integrity

ENKU's persistence architecture uses explicit records, checkpointing and A/B + CRC recovery for important state.

This is primarily an engineering/reliability advantage, not a marketing checkbox.

#### Dock Mode

Planned:
- clock/date/battery;
- book cover + reading progress;
- reading goal;
- custom text;
- cached weather;
- dashboard presets.

This can become a distinctive feature if e-paper ghosting and refresh behavior are measured and controlled.

See:
docs/dock-mode.md

#### Open hardware + kits

This remains ENKU's strongest structural differentiator versus BOOX/Kobo/PocketBook/XTEINK.

### Areas where ENKU must reach parity

Before calling the software “reader-complete”:
- EPUB quality;
- FB2 quality;
- TXT;
- TOC;
- bookmarks;
- in-book search;
- robust progress restore;
- grid/list library;
- sorting/search;
- typography presets/custom;
- stable sleep/wake;
- Wi-Fi transfer.

### Areas where ENKU is currently behind

#### PDF

Reader v1 currently defers PDF. XTEINK, BOOX, Kobo and PocketBook all have some PDF support.

Recommendation:
Do not block v1 on PDF, but publish the limitation clearly.

#### Dictionaries / translation

BOOX and PocketBook are materially stronger here.

Recommendation:
Add a local dictionary roadmap after the core reader is stable. This is more relevant to reading than adding unrelated apps.

#### Audiobooks / TTS

PocketBook and Kobo have mature audio-related features. ENKU intentionally does not target this in R0.x.

Recommendation:
Keep it out unless user demand is strong enough to justify codec/audio/Bluetooth UX complexity.

#### DRM / bookstore ecosystem

Kobo/PocketBook are much stronger for mainstream consumers.

Recommendation:
Do not chase them. ENKU should be positioned as an open/local reader rather than an alternative storefront.

#### Waterproofing

Kobo Clara BW and PocketBook Verse Pro have IPX8-class positioning.

Recommendation:
Do not target waterproofing for open kits / R0.x. It conflicts with repairability, accessible microSD and DIY assembly.

## Product variant positioning

### ENKU Electronics Kit

Audience:
makers, developers, open-hardware users.

Must stay below or near Inkplate-class pricing while offering a more reader-specific package.

### ENKU Base

Audience:
users who want a tiny physical-control reader without frontlight.

Must remain price-competitive with XTEINK X4 Classic and materially below mainstream 6-inch readers.

### ENKU Cover

Value comes from:
- Hall wake/sleep;
- real screen protection;
- same core electronics;
- no separate PCB.

### ENKU Pro

Must use a bonded frontlight panel rather than a fragile DIY optical stack.

Primary display candidate:
GDEY0426T82-FL01C.

Pro should not exist merely as “Base + light”; its value proposition is:
- frontlight;
- Dock;
- motion/orientation;
- cover support;
- open hardware;
- local software.

### ENKU Pro Wireless

Qi should remain a separate population / kit option.

Reason:
- protects Base and Pro BOM;
- avoids forcing heat/thickness/certification cost on all users;
- creates a legitimate premium tier.

### ENKU Dock

Dock should be inexpensive hardware with useful software value.

First revision:
- passive stand;
- 4-pin pogo;
- USB-C power input;
- no MCU;
- explicit dock detect.

The software, not dock electronics, creates most of the differentiation.

## BOM freeze rules from this analysis

Do not freeze the custom mainboard BOM unless:

1. Electronics Kit target <= PLN 155 remains plausible.
2. Base landed target <= PLN 180 remains plausible.
3. Pro frontlight target <= PLN 230 remains plausible.
4. Pro Wireless target <= PLN 260 remains plausible.
5. Dock target <= PLN 40 remains plausible.
6. One mainboard can still serve Base / Cover / Pro through population options.
7. Pro panel sourcing for GDEY0426T82-FL01C is confirmed at useful quantities.
8. Frontlight driver / optical requirements are validated from the actual panel specification.
9. Qi remains DNP / optional.
10. Software roadmap prioritizes reader fundamentals over feature count.

## Strategic conclusion

The market does not invalidate ENKU, but it removes the easy positioning.

Cheap finished pocket readers already exist.

ENKU only makes sense if it becomes a distinctly **open, local-first, physical-first pocket reader platform** rather than a more expensive imitation of XTEINK or BOOX.

The main product advantages worth protecting during PCB/BOM work are:

- open hardware and firmware;
- reproducible kits;
- clean one-hand physical UX;
- local web library management;
- motion/orientation;
- Dock Mode;
- flexible Base / Cover / Pro / Qi population;
- repairability and transparent ownership.

The detailed working spreadsheet is maintained separately as:
ENKU_Competitive_Market_Software_BOM_Targets_2026-10.xlsx
