# ENKU Reddit launch package — r/ereader

Status: ready to post manually.

## Recommended title

I'm building a tiny open-source e-reader around a 3.97" E Ink panel — what would you change before I freeze the hardware?

## Final post

Hi r/ereader,

I've been working on **ENKU**, a small open-source e-reader built around a 3.97-inch 800×480 e-paper display and an ESP32-S3.

It originally started as a firmware/UI experiment on a Waveshare development board, but I've now taken it far enough that I'm designing a dedicated PCB for it. Before I freeze the first hardware revision, I'd really like feedback from people who actually use e-readers.

The design is intentionally narrow:

- 3.97-inch e-paper display;
- physical controls rather than a touch-first interface;
- EPUB / FB2 / TXT;
- microSD storage;
- local Wi-Fi upload/library management;
- offline-first operation;
- no required account or proprietary book store;
- open firmware and open hardware;
- printable/modifiable enclosure;
- repairability considered in the PCB layout.

The software is already more than a screen mock-up. The project has a reader runtime, library state, pagination/parser work, progress persistence, removable-storage recovery, Wi-Fi degradation/recovery behavior, power/recovery states and an ESP-IDF hardware target.

I'm currently finishing **R0.1**, the first custom ENKU mainboard. I'm being deliberately conservative with the fabrication gate: passing DRC alone isn't enough. USB, power/dock routing, zero unrouted nets, return paths, antenna clearance, connector mechanics, footprints and the manufacturing outputs all need to be checked before I order boards.

The project is here if anyone wants to inspect the implementation rather than just the renders:

**https://github.com/raznoglaz1y/enku**

A few questions I'd genuinely like opinions on before I lock the hardware:

1. Does a ~4-inch reader make sense to you as a truly pocketable second reader, or is 6 inches your practical minimum?
2. On a device this small, which trade-off would you choose: thinner body, larger battery, or easier repairability?
3. Would you prefer three/four very good physical controls, or more dedicated buttons?
4. Is Wi-Fi upload + microSD enough for book transfer, or would USB mass-storage access be important?
5. What is one small thing your current reader does badly that you'd want a tiny reader like this to get right?

Happy to share more of the PCB or firmware architecture if that's useful.

## Why this version

r/ereader rules prohibit advertising/spam. This first post therefore deliberately:
- does not mention Kickstarter;
- does not mention pricing, pre-orders, mailing lists, "follow us", or purchase availability;
- links only to the open-source repository;
- asks concrete product/engineering questions;
- clearly labels R0.1 as unfinished hardware.

Do not add commercial CTA language in the comments unless the moderators explicitly permit it.

## Media order

### 1 — Hero / first image
Current approved ENKU product render.

Production asset:
https://enkureader.com/assets/enku-reader-approved.webp

Purpose: immediately communicate size/form factor. Caption it as a **design render**, not a finished product photo.

### 2 — UI
Canonical Library UI board:
https://raw.githubusercontent.com/raznoglaz1y/enku/main/design/screens/en/ENKU_library_grid_v2.svg

Purpose: show that the interaction/UI work is real and non-touch.

### 3 — UI / reader controls
Quick typography / reader overlay:
https://raw.githubusercontent.com/raznoglaz1y/enku/main/design/screens/en/ENKU_quick_aa_preview_v4.svg

Purpose: show e-paper-first UI detail rather than generic product branding.

### 4 — PCB
Use the newest R0.1 PCB screenshot/export from the current KiCad main branch after routing closure. Do not use an older placement-only render. If the final board preview is not yet generated, omit this slide rather than showing stale routing.

### 5 — Physical prototype
Add a real photo of the Waveshare 3.97-inch reference-board prototype once it is visually presentable. A real bench photo is more valuable than another render.

If there is no suitable current real photo, launch with 3–4 images rather than filling the carousel with weak material.

## First-comment / FAQ replies

### “When can I buy it?”
Not for sale yet. I'm still validating the first custom hardware revision, and I don't want to promise a date before the board has been fabricated and tested. The design and firmware are being developed openly in the meantime.

### “How much will it cost?”
I have BOM targets, but I don't think a public price means much until the first board has been assembled and the sourcing/assembly costs are real. The current priority is validating the hardware, not setting a retail number.

### “Why only 3.97 inches?”
The small size is the point of this particular project. I'm trying to find the lower end of the size range that is still comfortable for normal books while becoming genuinely pocketable. That's also one of the trade-offs I want feedback on.

### “Why ESP32-S3 instead of Linux?”
I want fast boot/wake, low idle power, a small understandable software stack and hardware that people can realistically inspect and modify. It obviously gives up some flexibility compared with Linux, so the firmware is intentionally focused on reading rather than becoming a general-purpose tablet.

### “Why no touchscreen?”
Physical controls are a product constraint rather than a missing feature. They simplify the interaction model, reduce accidental input and fit the small screen. The UI is being designed around deterministic focus navigation from the beginning.

### “Will it support PDF?”
Not in Reader v1. EPUB, FB2 and TXT are the current scope. PDF on a 3.97-inch 800×480 display is a very different problem, and I would rather make the core reading experience good than claim a format that is unpleasant to use.

### “Can it use Kindle books / DRM?”
The project is centered on user-owned/local files. I am not building a proprietary storefront or trying to bypass DRM.

### “Is it just a Waveshare board in a case?”
The Waveshare ESP32-S3-ePaper-3.97 is the current reference platform for bring-up. ENKU R0.1 is a dedicated mainboard with its own power, controls, storage/display integration and product-specific hardware decisions.

### “Is the render real?”
It is a design render. I will label it as such. The project separates design previews, source-verified work and physically validated hardware rather than presenting renders as finished devices.

### “Why another open-source e-reader?”
Because I want a very small physical-button reader with a deliberately narrow, offline-first feature set, and I want the hardware, firmware, UI rules and enclosure to be reproducible as one project rather than a black box.

## After posting

Record:
- upvotes after 2 h / 24 h / 72 h;
- comment count;
- GitHub stars delta;
- repeated feature requests;
- repeated objections;
- size/battery/control preference;
- questions about availability/price;
- any mod feedback.

Do not argue with criticism. Repeated criticism is product research.
