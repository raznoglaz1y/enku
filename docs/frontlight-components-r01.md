# ENKU Pro R0.1 — frontlight component freeze pass

> **SUPERSEDED / DO NOT FABRICATE — provenance review 2026-10-05**
>
> This component freeze is no longer an approved ENKU production baseline. The 10 µH / 15 Ω / low-side-selector pass followed review of the CERN-OHL-S-2.0 Silkscreen hardware project and is therefore quarantined rather than presented as independently developed ENKU hardware. Keep these values only as historical context until a clean manufacturer-source re-derivation replaces this file's fabrication role.


Status: **prototype electrical values selected / remap network removes 6-pin mapping as a PCB-respin blocker**

## Goal

Turn the chosen single-boost frontlight architecture into a concrete prototype BOM without overfitting to an unverified third-party FPC mapping.

## Core driver

**U_FL — TI TPS923610DRLR**

Selected because it provides:
- synchronous boost;
- 2.5–5.5 V input;
- up to 24.5 V output;
- 200 mV feedback reference;
- analog current dimming through ADIM;
- very low shutdown current;
- compact SOT-563-6 package.

TI:
https://www.ti.com/product/TPS923610

## Boost inductor

Prototype baseline:

**L_FL — TDK VLS252012HBX-100M-1**
- 10 µH;
- 2.5 × 2.0 mm class;
- ~850 mA saturation/current class;
- 1.2 mm height.

Why:
- compact enough for the ENKU Pro stack;
- already used successfully in a separately reviewed open-hardware design around the same TPS923610 driver class;
- sufficient current margin for a ~13 mA, ~15 V LED load from a 1-cell system rail.

LCSC:
https://www.lcsc.com/product-detail/C88532.html

This is a prototype baseline, not a permanent single-source requirement.

## LED current set

**R_FL_SET — 15 Ω / 1% / 0603**

TPS923610 regulates near 200 mV at FB.

Nominal current:
```text
I_LED ≈ 0.2 V / 15 Ω ≈ 13.3 mA
```

This deliberately stays below Good Display's <=15 mA panel limit.

Preferred value class:
- 15 Ω;
- 1%;
- 0603;
- >=0.1 W.

The current limit must be rechecked on the actual panel lot.

## Warm / cool low-side selectors

Prototype baseline:
**Q_FL_WARM / Q_FL_COOL — BSS138-family N-MOSFET**

Requirements:
- >=30 V VDS preferred; 50 V class is comfortable;
- low gate threshold at 3.3 V;
- SOT-23;
- LED branch current is only ~13 mA, so conduction loss is negligible.

Reference quality candidate:
**onsemi BSS138LT1G**

Cost-down equivalents may be qualified later.

## Output capacitor

Prototype baseline:

**C_FL_OUT — 4.7 µF / 50 V / 0805 / X5R or X7R**

Reason:
- enough voltage margin for the 15 V-class frontlight rail;
- aligns with TI's 1–4.7 µF recommended output-capacitance window;
- preserves useful effective capacitance under DC bias better than a nominal 1 µF 0805.

Candidate class:
- Samsung CL21A475KBQNNNE
- or equivalent qualified 4.7 µF / 50 V 0805 X5R/X7R.

Do not substitute a lower-voltage MLCC.

## Input capacitor

Use a local input capacitor close to TPS923610:
- 4.7 µF;
- 10–16 V;
- X5R/X7R;
- plus 100 nF local ceramic.

Final source rail depends on the exact power-tree sheet.

## Brightness input

`FL_ADIM` / `FL_PWM` drives ADIM.

Firmware requirements:
- explicit startup pulse >40 µs before normal dimming;
- hold ADIM low long enough to enter shutdown;
- do not rely on tiny PWM pulses to wake the driver.

## Colour selection architecture

Two options remain footprint-compatible:

### Option A — two independent GPIO controls

Populate:
- Q_FL_WARM
- Q_FL_COOL

Firmware guarantees mutually exclusive selection.

Pros:
- easiest bring-up;
- supports both-off;
- easiest scope/debug.

### Option B — one GPIO + inverter

Add:
**74LVC1G04**-class inverter

Steady-state behavior makes exactly one branch active.

Pros:
- one GPIO;
- protects against steady-state double-on.

Cons:
- colour-transition overlap/both-off must still be scoped.

For R0.1, keep two control footprints and allow the inverter as DNP/alternate unless pin pressure makes it necessary.

## Frontlight connector

Connector class:
- 6-pin;
- 0.5 mm pitch;
- horizontal ZIF/FPC.

Candidate family:
**Hirose FH34SRJ-6S-0.5SH(50)**

Use the dual-contact **Hirose FH34SRJ-6S-0.5SH(50)** so FPC contact side is no longer a PCB-layout blocker.

R0.1 keeps the connector pins as raw nets:
- FL_RAW1
- FL_RAW2
- FL_RAW3
- FL_RAW4
- FL_RAW5
- FL_RAW6

A small 0 Ω remap network connects the raw pins to:
- FL_LED_PLUS
- FL_WARM_RETURN
- FL_COOL_RETURN
- unused/DNP positions.

This lets the first board adapt to the delivered panel pin order without a PCB respin. The production revision should collapse the remap network once the actual panel lot is verified.

## Protection

Recommended prototype protections:
- output TVS only if it does not add unnecessary capacitance/size and is justified by enclosure/accessibility;
- ESD protection on externally exposed FPC paths is optional because the panel FPC is internal;
- output over-voltage primarily relies on TPS923610 OVP.

Do not copy external-accessory protection from unrelated dev boards unless the same exposure exists in ENKU.

## Cost direction

The complete frontlight electronics should remain a small fraction of the Pro premium.

Expected high-level cost contributors:
- TPS923610: a few PLN;
- 10 µH inductor: sub-PLN to low-PLN;
- 2 × small MOSFETs: sub-PLN;
- passives/connector: a few PLN.

The **bonded Pro panel and enclosure mechanics**, not the driver circuit, should dominate the Pro BOM delta.

## Prototype freeze state

| Item | State |
| --- | --- |
| TPS923610 | selected |
| 10 µH inductor value | selected |
| VLS252012HBX-100M-1 | prototype candidate |
| 15 Ω current-set | selected |
| BSS138 branch FETs | prototype candidate |
| 4.7 µF / 50 V output cap | selected |
| 6-pin connector class | selected |
| exact FL pin map | first-article validation; remap network prevents PCB respin |
| continuous warm/cool blending | test item |
| exact production alternates | post-prototype cost-down |

## PCBWay gate

The common PCB may proceed with the dual-contact connector and remap network.

Before locking the **production** Pro population, verify:
- delivered panel pin mapping;
- LED polarity;
- current direction;
- warm/cool channel identity.

For R0.1, populate the remap resistors only after continuity-checking the first panel sample.
