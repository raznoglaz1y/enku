# ENKU Pro R0.1 — frontlight strategy

Status: **prototype strategy / not production-frozen**

## Display

Primary Pro panel:
**Good Display GDEY0426T82-FL01C**

Current vendor data:
- 4.26 inch;
- 800 × 480;
- SSD1677;
- bonded frontlight;
- 7 LEDs;
- warm + cool light;
- <= 15 V operating voltage;
- <= 15 mA operating current;
- separate 6-pin frontlight FPC.

Source:
https://www.good-display.com/product/880.html

## Prototype approach

Use two independent current-controlled boost channels initially.

Why:
- easiest way to validate warm / cool strings independently;
- easy PWM mixing;
- faults on one channel do not automatically disable the other;
- lets firmware tune color temperature without a complex multichannel controller.

Reference candidate:
**2 × TPS61165**

This is intentionally a quality-first prototype approach.

## Production cost-down

Before Kickstarter / 100-unit production:
- price a dual-channel WLED driver;
- compare total cost including inductors, diodes, capacitors and placement count;
- compare minimum dimming level and audible/visible PWM artifacts;
- retain two-driver design if the saving is too small to justify extra risk.

## Firmware control target

Expose:
- brightness 0–100%;
- warmth 0–100%;
- quick-menu brightness;
- low-light preset;
- frontlight off in sleep;
- optional restore-last-brightness.

Suggested model:

```text
warm_current = brightness × warmth
cool_current = brightness × (1 - warmth)
```

Apply gamma / perceptual mapping in firmware so low brightness has useful resolution.

## UX rule

The minimum usable brightness matters more than maximum brightness.

A Pro reader that is too bright at level 1 is a worse night-reading product even if its maximum lumen output looks impressive in specifications.

## Thermal rule

Frontlight validation must be done:
- at maximum warm;
- maximum cool;
- mixed maximum;
- while charging;
- with the final enclosure.

Do not assume the panel's <=15 mA rating means the enclosure will remain comfortable at continuous maximum brightness.

## Final freeze blocker

Do not assign production pin numbers or final current set resistors until the current GDEY0426T82-FL01C frontlight FPC drawing/specification has been checked.
