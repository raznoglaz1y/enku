# ENKU Reddit launch draft — r/ereader

## Recommended title

I'm building ENKU — a compact open-source 4-inch e-reader. I'd love feedback before I freeze the hardware.

## Post

I've been working on **ENKU**, a compact open-source e-reader built around a 3.97-inch 800×480 e-paper display and ESP32-S3.

It started as a firmware/UI experiment on a Waveshare development board, but the project has now grown into a complete open hardware device. I'm currently finishing the first custom ENKU mainboard, so this feels like the right moment to ask people who actually use e-readers what matters before the hardware is frozen.

The basic idea is deliberately simple:

- small 3.97-inch e-paper display;
- physical controls instead of a touchscreen-dependent UI;
- EPUB / FB2 / TXT reading;
- microSD storage;
- local Wi-Fi uploader and library management;
- no required account;
- no proprietary bookstore or cloud dependency;
- offline-first behavior;
- open firmware;
- open hardware;
- printable / modifiable enclosure;
- repairability and accessible parts considered in the PCB design.

The firmware is already well beyond a mock-up: the reader runtime, library behavior, removable-storage recovery, local management path, power/recovery states and a large part of the device architecture are implemented and host-tested.

At the same time I'm finishing **ENKU R0.1**, the first dedicated PCB. I'm intentionally keeping the fabrication gate strict: clean native KiCad checks alone are not enough — USB, dock/power routing, zero unrouted connections, return paths, footprints, mechanics, repairability and the manufacturing package all need to pass before I send the board out.

The project is being developed publicly and the repository contains the firmware, hardware work, UI documentation and build log:

**GitHub:** https://github.com/raznoglaz1y/enku  
**Project site:** https://enkureader.com

I'm also considering a small production run / crowdfunding later, but I don't want to design the hardware around a campaign. I want to get the reader right first.

A few things I'd especially like feedback on:

1. For a truly pocketable reader, does ~4 inches feel useful to you, or would you personally never go below 6 inches?
2. Which matters more on a device like this: minimum thickness, larger battery, or easier repairability?
3. Would you rather have a very small set of excellent physical controls, or more dedicated buttons?
4. Is local Wi-Fi upload + microSD enough, or is USB mass-storage access important to you?
5. If this eventually becomes a kit / small production run, would you prefer a fully assembled reader or an electronics + printable-case option?

I'm happy to share more of the PCB, UI or firmware architecture if anyone is interested.

## Media order

1. Best current ENKU product render.
2. Photo of the Waveshare-based development prototype when available.
3. Library / reader UI board.
4. R0.1 PCB screenshot / render.
5. Optional architecture or exploded-view image.

## Posting notes

- Do not lead with Kickstarter.
- Do not use marketing language such as "revolutionary", "best", "coming soon", or price promises.
- Reply technically and transparently.
- If people ask where they can buy it, answer that hardware is still in validation and invite them to follow GitHub/site.
- Save repeated questions; they become product requirements and Kickstarter FAQ material.
