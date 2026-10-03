# ENKU R0.1 — display freeze matrix

Status: **current products verified / connector orientation and exact HV parts still blocked**

This document separates what is now verified from Good Display's current 2026 product data from what still needs direct drawing/spec inspection before PCB fabrication.

## Current Base panel

**GDEY0397T81P**

Good Display currently lists:

- 3.97 inch
- 800 × 480
- SSD1677
- 24-pin FPC
- 0.5 mm pitch
- outline: 96.62 × 56.24 × 0.92 mm
- active area: 86.40 × 51.84 mm
- pixel pitch: 0.108 × 0.108 mm
- 4 grayscale levels
- full update: 3 s
- fast refresh: 1.5 s
- partial refresh: 0.3 s
- typical refresh power: 28.2 mW
- deep-sleep power: 0.003 mW
- weight: 10.45 ± 0.5 g

Current manufacturer specification file is dated **2026-08-13**.

Official product:
https://www.good-display.com/product/613.html

## Current Pro / Pro Wireless panel

**GDEY0426T82-FL01C**

Good Display currently lists:

- 4.26 inch
- 800 × 480
- SSD1677
- 24-pin EPD FPC
- 0.5 mm pitch
- outline: 105.33 × 62.37 × 1.8 mm
- active area: 92.8 × 55.68 mm
- pixel pitch: 0.116 × 0.116 mm
- 4 grayscale levels
- full refresh: 3.5 s
- fast refresh: 1.5 s
- partial refresh: 0.42 s
- refresh power: 26.4 mW
- standby power: 0.0066 mW
- separate 6-pin / 0.5 mm frontlight FPC
- 7 frontlight LEDs
- cool + warm channels
- series-connected light strings
- operating voltage <= 15 V
- operating current <= 15 mA
- weight: 18.5 ± 0.5 g

Current manufacturer specification file is dated **2026-09-01**.

Good Display also currently publishes:
- GDEY0426T82-FL01C mechanical drawing;
- FL0426-S01C frontlight drawing;
- CV0426 module CAD drawing.

Official product:
https://www.good-display.com/product/880.html

## Mechanical delta: Base vs Pro

The Pro display is approximately:

- 8.71 mm wider;
- 6.13 mm taller;
- 0.88 mm thicker;
- 8.05 g heavier.

Implication:

The common ENKU PCB strategy is still plausible electrically, but the enclosure/display carrier cannot be treated as one identical mechanical stack.

Base and Pro should share:
- core PCB if FPC compatibility is confirmed;
- control philosophy;
- battery/power architecture;
- firmware interfaces.

They may require different:
- front bezel;
- panel pocket;
- display support geometry;
- optical/frontlight stack clearance;
- final battery placement.

## What is verified enough to use now

### Safe for schematic architecture

- SSD1677 controller family;
- 24-pin / 0.5 mm EPD connector class;
- 4-wire SPI architecture;
- external EPD-HV circuit requirement;
- Base and Pro display dimensions;
- Pro separate frontlight FPC;
- Pro warm/cool series frontlight architecture;
- <=15 V / <=15 mA frontlight envelope.

### Not safe to fabrication-freeze yet

- FPC contact side;
- connector insertion direction;
- exact pin numbering of both current GDEY variants;
- exact Pro 6-pin warm/cool assignment;
- exact inductor;
- exact HV MOSFET;
- exact Schottky parts;
- final RESE value;
- final HV capacitor count/value;
- any routing that depends on the unverified current panel drawing.

## Current decision

Continue schematic capture now using named logical nets and a clearly marked EPD-HV reference block.

Do **not** authorize PCBWay fabrication until the current Good Display PDF/drawing pin tables are directly checked and the provisional EPD-HV block is replaced by exact parts.
