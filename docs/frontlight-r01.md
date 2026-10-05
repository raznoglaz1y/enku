# ENKU Pro R0.1 — frontlight strategy

> **PROVENANCE / FABRICATION HOLD — 2026-10-05**
>
> The October 2 single-boost reference pass was informed by review of the Silkscreen open-hardware reader. Because Silkscreen hardware source is published under CERN-OHL-S-2.0, ENKU does **not** treat the resulting TPS923610 + 15 Ω + 10 µH + low-side-selector combination as clean ENKU-owned fabrication source. This sheet is retained as engineering history only.
>
> **R0.1 rule:** frontlight population is DNP / not fabrication-authoritative until the block is independently re-derived from manufacturer sources (Good Display panel data/drawing and the LED-driver manufacturer's datasheet/application guidance), reviewed for provenance, and physically validated. Do not copy Silkscreen schematic/layout expression, reference designators, protection network, or component-selection rationale into the replacement.
>
> Using TPS923610 itself is not prohibited: it is a TI catalog component and can be selected independently. The replacement design must document its own calculations and source trail.


Status: **quality/cost architecture selected / current 6-pin panel mapping still requires direct manufacturer-drawing verification**

## Display

Primary Pro panel:
**Good Display GDEY0426T82-FL01C**

Current Good Display product data:
- 4.26 inch;
- 800 × 480;
- SSD1677;
- bonded frontlight;
- 7 LEDs;
- warm + cool light;
- series-connected strings;
- <= 15 V operating voltage;
- <= 15 mA operating current;
- separate 6-pin / 0.5 mm frontlight FPC.

Source:
https://www.good-display.com/product/880.html

## R0.1 driver decision

The earlier 2 × TPS61165 concept is replaced by a **single synchronous boost driver plus warm/cool return selection**.

Primary driver candidate:

**TI TPS923610DRLR**

Why:
- active production part;
- 2.5–5.5 V input;
- up to 24.5 V output;
- 200 mV current feedback;
- integrated synchronous switches;
- analog current dimming controlled by PWM;
- down to 0.1% dimming ratio;
- 130 nA typical shutdown current;
- OVP / OCP / UVLO / thermal shutdown;
- tiny SOT-563-6 package;
- current unit price around USD 0.50–0.65 in small quantities.

TI:
https://www.ti.com/product/TPS923610

LCSC:
https://www.lcsc.com/product-detail/C52919131.html

This is both cleaner and cheaper than two independent boost converters.

## Channel architecture

Preferred concept:

```text
                TPS923610
VSYS/rail ---> boost ---> LED+
                       /      \
                 warm string  cool string
                     |            |
                   Q_WARM       Q_COOL
                     \            /
                      +--- RSET --- GND
```

Both strings share:
- one boost;
- one current sense resistor;
- one brightness control.

Warm/cool selection is done on the return side.

This avoids paying for two boost stages while keeping the two colour strings separately selectable.

## Current ceiling

TPS923610 regulates approximately 200 mV on FB.

Target full-brightness current:
**~13 mA**, intentionally below Good Display's <=15 mA limit.

Initial sense resistor target:

```text
RSET = 0.2 V / 0.013 A ≈ 15.4 Ω
```

Prototype choice:
**15.0 Ω or 15.4 Ω / 1%**, selected after exact panel current requirements and resistor tolerance are checked.

Do not target the absolute 15 mA panel maximum for production.

## Brightness control

Use TPS923610 ADIM for brightness.

TI specifies:
- PWM-controlled analog dimming;
- output current follows PWM duty rather than hard LED chopping;
- 0.1% dimming ratio;
- first enable pulse >40 µs;
- ADIM low for ~2.5 ms shuts the part down.

Firmware should therefore:
1. issue a deliberate enable pulse;
2. then start the brightness waveform;
3. hold ADIM low for shutdown.

This should give much better low-light behavior than crude LED-side PWM.

## Warm / cool control

R0.1 should initially prioritize **safe discrete colour selection** over fancy continuous blending.

Two possible implementations:

### A. Two GPIO-controlled low-side FETs
- FL_WARM_SEL
- FL_COOL_SEL

Firmware guarantees only one channel is on.

Advantages:
- simple;
- easiest to validate;
- allows both-off explicitly.

Risk:
- firmware fault can turn both strings on.

### B. One GPIO + hardware inverter
One FET gets COLOR_SEL directly, the other gets its inverse.

Advantages:
- steady-state hardware prevents both strings being on at once;
- saves one GPIO.

Risk:
- brief overlap/both-off during transitions still needs measurement;
- exact continuous colour blending becomes a timing problem.

**R0.1 preference:** reserve footprints so both strategies can be evaluated, but populate the simpler safe configuration after the actual 6-pin FPC mapping is confirmed.

## External cross-check

A separate 2026 open-hardware ESP32-S3 reader project targeting the same 4.26-inch Good Display family independently converged on:
- TPS923610;
- one boost rail;
- ~13.3 mA current ceiling;
- low-side warm/cool selection.

This is useful evidence that the architecture is sensible, but that project's author explicitly marks its frontlight blend behavior and sample FPC mapping as not yet fully hardware-validated. It is therefore a cross-check, not ENKU's source of truth.

ENKU still requires the current Good Display FL0426-S01C drawing.

## Frontlight connector

The panel uses a 6-pin / 0.5 mm FPC.

Do **not** freeze the connector net order from third-party sample mappings.

The current Good Display FL0426-S01C drawing is authoritative.

Until directly verified:
- label connector pins FL_PIN1..FL_PIN6 in schematic capture;
- keep warm/cool nets on a clearly isolated sub-sheet;
- do not send the Pro population to PCBWay.

## Power source

Frontlight must not bypass hard OFF.

Preferred source:
- VSYS or another rail that is physically disabled by the system hard-off architecture;
- not raw battery directly unless a separate hard-off load switch is present.

The TPS923610 itself has very low shutdown current, but the product requirement remains that Hard OFF genuinely removes the user-facing system loads.

## Cost impact

At current distributor pricing the TPS923610 IC is roughly:
- ~USD 0.52–0.64 at 10 units;
- ~USD 0.50–0.63 around 100 units.

That is only a few PLN.

Therefore the frontlight BOM cost is dominated more by:
- bonded Pro panel premium;
- inductor/passives;
- FPC connector;
- enclosure/display-stack differences;
than by the LED driver IC.

The single-driver approach protects Pro margin without compromising driver quality.

## Optical / thermal validation

Validate:
- minimum usable brightness;
- maximum warm;
- maximum cool;
- mixed/intermediate CCT if implemented;
- while charging;
- in final enclosure;
- at low battery;
- after extended overnight use.

The minimum usable brightness matters more than maximum brightness for a reader.

## Freeze gates

Before production freeze:

- current FL0426-S01C 6-pin mapping directly verified;
- actual warm/cool forward voltage measured;
- RSET validated below 15 mA maximum;
- boost inductor selected;
- output capacitor effective capacitance checked under bias;
- low-side selector FETs chosen;
- transition behavior measured;
- minimum brightness judged acceptable in darkness;
- frontlight off-current measured in Sleep and Hard OFF.


## Component freeze companion

Exact prototype component candidates and cost-down notes are tracked here:

[Frontlight components R0.1](frontlight-components-r01.md)


## Frontlight remap safety

R0.1 will not hard-wire an unverified 6-pin sample mapping.

The six raw FPC pins are routed through a small 0 Ω / solder-jumper remap matrix before becoming:
- FL_LED_PLUS;
- FL_WARM_RETURN;
- FL_COOL_RETURN.

Together with the dual-contact Hirose 6-pin connector, this removes the FPC mapping uncertainty as a PCB-spin blocker while keeping the production revision easy to simplify after sample validation.

See:
[R0.1 blocker closure](r01-blocker-closure.md)
