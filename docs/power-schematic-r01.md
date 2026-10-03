# ENKU custom mainboard R0.1 — power sheet candidate

Status: **schematic-level baseline / not fabrication-frozen**

This document turns the current power architecture into a concrete first schematic target.

The goal is not to maximize charging speed. It is to get a predictable, low-risk power path that works for Base / Cover / Pro / Pro Wireless and preserves margin through a common PCB.

## 1. Core architecture

```text
USB-C VBUS ----\
                > input selector / protected OR ---> BQ25185 IN
Dock 5V -------/
Qi 5V ----------> optional Pro Wireless input path

BQ25185 BAT  ---> 1S Li-Po
BQ25185 SYS  ---> TPS63802 VIN
TPS63802 3V3 ---> ESP32-S3 / microSD / logic / control

Hard OFF ---> TPS63802 EN low
```

Important consequence:

The physical hard-off switch no longer needs to interrupt the battery itself.

BQ25185 remains connected to the battery and external power, so charging continues while the system is hard-off.

TPS63802 provides true shutdown / load disconnect, so it is a strong candidate for making the 3.3 V digital rail genuinely off.

EPD-HV and frontlight rails must also be disabled in hard-off. They may not bypass the switched system architecture.

## 2. BQ25185 baseline values

Primary charger:
**BQ25185DLHR**

Official reference:
https://www.ti.com/product/BQ25185

### Battery regulation / input current limit

Initial ENKU target:
- 1-cell Li-Po;
- 4.2 V battery regulation;
- 500 mA external input-current limit.

TI's published design procedure gives:

- **R_ILIM/VSET = 18 kΩ** for 4.2 V regulation and 500 mA input-current limit.

This is the R0.1 starting value.

### Fast-charge current

Prototype target:
- **300 mA initial fast-charge current**

TI's published design example gives:
- **R_ISET = 1 kΩ** for 300 mA fast charge.

Why start at 300 mA:
- lower thermal load on a compact PCB;
- friendly to the current ~2000 mAh battery target;
- safer before the final cell and enclosure thermal path are known.

After the actual battery is selected and temperature is measured, a 500 mA charge profile can be evaluated.

Do not raise charge current just to advertise a faster number.

### Capacitors

Follow the current TI recommendation:
- IN: at least 1 µF after derating;
- SYS: nominal 10 µF, at least 1 µF after derating;
- BAT: at least 1 µF.

R0.1 placement target:
- C_IN: 4.7 µF, X7R/X5R, **25 V**, plus 100 nF;
- C_SYS: 10 µF, X7R/X5R, **25 V**, plus 100 nF;
- C_BAT: 4.7 µF, X7R/X5R, 10 V or higher.

TI specifically recommends considering 25 V-rated ceramic capacitors on IN and SYS because of DC-bias derating.

### CE

/CE is active low.

Baseline:
- default hardware pull-down so charging works without firmware;
- optional GPIO override/test point if later useful.

Charging must not depend on the ESP32 being powered.

### STAT

STAT1 / STAT2 are open-drain outputs.

R0.1:
- expose at least one status signal to an ESP32 GPIO or test pad;
- avoid mandatory always-on LEDs;
- production status LED remains DNP to protect standby power.

### TS / battery temperature

Production target:
- use a real battery NTC / validated thermal network.

Do not ship a production kit with a fixed resistor that defeats battery temperature protection merely to save BOM.

For prototypes, an explicit documented test resistor may be used only while the final battery pack is unresolved.

## 3. 3.3 V rail

First-spin regulator:
**TPS63802DLAR**

Official reference:
https://www.ti.com/product/TPS63802

Why R0.1 moved away from TPS63031:
- TPS63031 has insufficient boost-mode margin for an intentionally simultaneous ESP32-S3 Wi-Fi + microSD + e-paper workload;
- ENKU should not depend on firmware preventing peak loads from overlapping;
- TPS63802 provides materially more transient/current headroom while preserving buck-boost operation from a 1S Li-Po.

R0.1 baseline:
- VIN: `VSYS`;
- VOUT: `3V3_SYS`;
- EN: `SYS_EN`;
- MODE: LOW for power-save mode;
- PG: `REG_PG`, pulled up with 100 kΩ for diagnostics;
- inductor: **Murata DFE201612E-R47M=P2, 0.47 µH**;
- input: 10 µF + local 100 nF;
- output: **2 × 22 µF**, X5R/X7R, with DC-bias derating checked;
- feedback: **511 kΩ / 91 kΩ** for the 3.3 V target.

Layout rule:
- input/output capacitors sit directly at the converter pins;
- L1/L2-to-inductor loop is extremely short;
- FB divider stays on the quiet side of the converter and away from L1/L2/SW copper;
- power ground and control ground join locally at the IC, not through a long shared return.

### EN / hard power

Preferred architecture:
- hard switch drives TPS63802 EN;
- EN OFF = `3V3_SYS` physically disabled;
- BQ25185 remains alive so battery charging still works.

Add:
- deterministic EN pull state;
- service/test pad on `SYS_EN`;
- test point on `REG_PG`;
- no firmware dependency for true hard-off.

### Current-margin gate

Before production freeze, stress the rail with:
- ESP32-S3 Wi-Fi transmit burst;
- microSD write;
- e-paper refresh;
- Pro frontlight control activity where applicable;
- battery near the low end of the allowed operating range.

Pass criteria are based on measured 3.3 V droop, reset margin and regulator temperature, not merely on nominal current sums.

## 4. USB-C and Dock source selector

Quality-baseline candidate:
**TPS2121**

Official reference:
https://www.ti.com/product/TPS2121

R0.1 intent:
- USB-C VBUS on IN1;
- Dock 5 V on IN2;
- OUT feeds BQ25185 IN;
- source selection configured so reverse current between USB and Dock is blocked;
- current limit chosen above ENKU's normal charging/system demand but below connector/trace safety limits.

Exact PR1 / CP2 / ILM component values are not frozen until the final source-priority behavior is selected.

### Cost-down checkpoint

Before production:
- compare TPS2121 against qualified discrete ideal-diode / MOSFET OR-ing.

Keep TPS2121 unless the discrete solution:
- saves a meaningful amount at 50–100 units;
- preserves reverse-current blocking;
- is safe with simultaneous USB + Dock;
- does not add more placement/test/support cost than it saves.

## 5. USB-C input

USB-C remains USB 2.0 device + charging input.

Keep:
- 5.1 kΩ Rd on CC1;
- 5.1 kΩ Rd on CC2;
- USB ESD protection;
- D+/D- series damping only if required by ESP32-S3 reference design/layout.

VBUS must go into the source-selection / charger path, not directly onto the system rail.

## 6. Dock interface

Four exposed contacts remain the target.

R0.1 electrical allocation:

1. GND
2. +5 V
3. DOCK_DETECT
4. GND

Reason for two ground contacts in R0.1:
- robust first-make / last-break behavior is easier to design mechanically;
- lower contact resistance;
- no need to invent a data bus before the accessory protocol exists.

Future revisions may reuse contact 4 for accessory ID only after mechanical validation.

DOCK_DETECT:
- resistor-coded / dedicated detection;
- input protection;
- GPIO with defined pull state;
- must distinguish Dock Mode from normal USB charging.

## 7. Pro Wireless Qi path

Preferred system architecture:

```text
Qi receiver -> regulated ~5 V -> common external-source path -> BQ25185
```

Candidate receiver:
**BQ51013C family**

Do not use a second independent battery charger unless there is a demonstrated reason.

### Three-source issue

TPS2121 has two inputs.

For Pro Wireless the architecture therefore needs one of:

A. USB + Dock through TPS2121, then ideal-diode OR with Qi before BQ25185;

B. USB + Dock passive/ideal-diode combination, with TPS2121 selecting wired vs Qi;

C. a different 3-input architecture if it proves cheaper/smaller.

This decision is intentionally not frozen yet.

## 8. Pro frontlight electrical envelope

Display:
**GDEY0426T82-FL01C**

R0.1 frontlight baseline:
- **TPS923610DRLR** boost LED driver;
- **TDK VLS252012HBX-100M-1, 10 µH**;
- **15 Ω / 1%** current-sense baseline;
- **4.7 µF / 50 V** output capacitor;
- BSS138-family warm/cool return selectors;
- master `FL_ENABLE` plus independent `FL_WARM_PWM` / `FL_COOL_PWM`;
- 6-pin dual-contact **FH34SRJ-6S-0.5SH(50)** connector;
- 18-position DNP 0 Ω remap matrix so first-article FPC continuity can be corrected without a PCB respin.

The exact warm/cool FPC mapping stays a first-article population decision until the delivered panel is continuity-checked.

## 9. E-paper HV status

The SSD1677 external HV topology is now captured in the R0.1 schematic.

Current 3.97-inch application-circuit baseline:
- `L_EPD`: **TYS5040100M-10, 10 µH**;
- `Q_EPD`: IRLML6346TRPBF;
- D1/D2/D3: MBR0530;
- `R_RESE`: 2.2 Ω / 1%;
- `R_GDR_PD`: 1 MΩ;
- local 4.7 µF booster input capacitor;
- rail capacitors on VGH/VGL/VSH1/VSH2/VSL/VCOM.

The older 47 µH TYS5040470M-10 is no longer the default. It remains a laboratory alternate only because it shares the TYS5040 land pattern.

Production still requires scope validation of VGH/VGL/VSH/VSL/VCOM on first hardware.

## 10. Power BOM freeze gates

Power sheet becomes production-frozen only after:

- final battery part selected;
- BQ25185 thermal charge test;
- USB + Dock simultaneous-source test;
- hard-OFF charging test;
- TPS63802 simultaneous peak-load / droop / thermal test;
- sleep leakage measurement;
- Hall wake/sleep validation;
- Pro frontlight dimming / thermal / optical test;
- Pro Wireless Qi thermal / alignment / interoperability test;
- EPD-HV first-article rail validation.

Until then, this document is the **preferred engineering baseline**, not a fabrication authorization.
