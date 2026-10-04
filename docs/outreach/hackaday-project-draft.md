# ENKU Hackaday.io project draft

## Project title

ENKU — Open-source pocket e-reader

## One-line description

A compact ESP32-S3 e-paper reader with physical controls, local ownership of books, open firmware, open hardware and a repairable custom mainboard.

## Description

ENKU is a compact open-source e-reader project built around a 3.97-inch 800×480 e-paper display and ESP32-S3.

The goal is not to reproduce a tablet or another cloud-connected bookstore device. ENKU is designed as a small, focused reader that remains useful offline and understandable all the way from UI behavior to the PCB.

The project began on the Waveshare ESP32-S3-ePaper-3.97 development platform. That board remains useful for reference bring-up, while a dedicated portrait-first ENKU mainboard is now being engineered in parallel.

Current project areas include:

- EPUB / FB2 / TXT reader architecture;
- non-touch UI and deterministic physical focus navigation;
- library, metadata, progress and per-book state;
- local browser-based upload and management;
- removable microSD recovery;
- Wi-Fi as an optional, degradable service rather than a reading dependency;
- explicit suspend / power-off behavior;
- custom R0.1 PCB;
- repairability and accessible components;
- printable enclosure work;
- reproducible manufacturing outputs.

The R0.1 fabrication gate is intentionally conservative. The board will not be released just because DRC passes: USB data, dock/power routing, zero unrouted nets, return paths, EPD high-voltage layout, antenna clearance, connector mechanics, footprints, repairability and the final Gerber/BOM/PnP package all have to be reviewed.

Long term, ENKU may become a small-batch kit or crowdfunding project. The open build remains the primary project: firmware, hardware documentation and printable mechanical work are intended to stay public.

Project repository: https://github.com/raznoglaz1y/enku  
Project site: https://enkureader.com

## Suggested tags

eink, e-paper, esp32-s3, ereader, open-source, open-hardware, epub, embedded, pcb, kicad

## First project logs

1. Why a 3.97-inch reader?
2. From Waveshare reference board to a dedicated ENKU PCB.
3. Reader firmware architecture: offline first.
4. R0.1 routing and the fabrication gate.
5. UI for e-paper and physical controls.
