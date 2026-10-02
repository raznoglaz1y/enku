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
BQ25185 SYS  ---> TPS63031 VIN/VINA
TPS63031 3V3 ---> ESP32-S3 / microSD / logic / control

Hard OFF ---> TPS63031 EN low
```

Important consequence:

The physical hard-off switch no longer needs to interrupt the battery itself.

BQ25185 remains connected to the battery and external power, so charging continues while the system is hard-off.

TPS63031 provides load disconnect during shutdown, so it is a strong candidate for making the 3.3 V digital rail genuinely off.

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

Primary candidate:
**TPS63031DSKR**

Official reference:
https://www.ti.com/product/TPS63031

TI fixed-3.3 V reference application:
- L = **1.5 µH**;
- input C = **10 µF**;
- VINA bypass = **0.1 µF**;
- output = **2 × 10 µF**;
- fixed 3.3 V output.

R0.1 should copy the vendor reference topology and verify exact recommended inductor / capacitor ESR and DC-bias requirements before purchasing parts.

### EN / hard power

Preferred architecture:
- hard switch drives TPS63031 EN;
- EN OFF = 3V3_SYS physically disabled;
- charger remains connected and active.

Add:
- deterministic EN pull state;
- service/test pad;
- optional RC only if power-up sequencing actually requires it.

### Current-margin gate

TPS63031 is only frozen after a bench stress test with:
- ESP32-S3 Wi-Fi transmit burst;
- microSD write;
- e-paper refresh activity;
- maximum expected logic load.

TI states up to 800 mA at 3.3 V in step-down conditions, but boost-mode capability is lower.

If this test shows insufficient margin, use a higher-current buck-boost rather than forcing a marginal design.

Fallback candidate:
**TPS63070**

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

Good Display currently specifies:
- 7 LEDs;
- warm + cool frontlight;
- frontlight voltage <= 15 V;
- frontlight current <= 15 mA;
- separate 6-pin frontlight FPC.

Reference:
https://www.good-display.com/product/880.html

### R0.1 quality-baseline driver concept

For prototype validation, reserve enough PCB area for **two independently dimmable boost LED channels**:
- warm channel;
- cool channel.

Reference single-channel driver candidate:
**TPS61165**

Relevant properties:
- 3–18 V input;
- up to 38 V output;
- PWM / one-wire brightness control;
- 200 mV current-sense reference;
- open-LED protection.

Reference:
https://www.ti.com/product/TPS61165

At 12 mA target current, the nominal sense resistor from the 200 mV reference is approximately:

```text
RSET = 0.2 V / 0.012 A ≈ 16.7 Ω
```

Use **16.9 Ω / 1%** as the first prototype value only after the new Good Display FPC pinout confirms that each color channel is an independently drivable series string.

Do not freeze the final frontlight driver until:
- the new GDEY frontlight FPC pinout is verified;
- warm/cool electrical topology is confirmed;
- optical minimum brightness is tested.

### Cost target

Two separate drivers are acceptable for the first Pro prototype because they minimize ambiguity and make warm/cool control independent.

For production, investigate a dual-channel driver if it can:
- reduce BOM/area;
- preserve very low minimum brightness;
- preserve separate warm/cool PWM;
- remain cheaper after passives and assembly.

## 9. E-paper HV status

Still a blocker.

The Base and Pro displays both use SSD1677-family interfaces, but the board must not invent the EPD high-voltage supply.

Freeze only after:
- Good Display reference circuit / adapter schematic is received or verified;
- all FPC pins are mapped;
- booster / gate / source rail component ratings are confirmed;
- layout constraints are known.

## 10. Power BOM freeze gates

Power sheet becomes production-frozen only after:

- final battery part selected;
- BQ25185 thermal charge test;
- USB + Dock simultaneous-source test;
- hard-OFF charging test;
- TPS63031 peak-load test;
- sleep leakage measurement;
- Hall wake/sleep validation;
- Pro frontlight dimming / thermal / optical test;
- Pro Wireless Qi thermal / alignment / interoperability test;
- official EPD-HV circuit validation.

Until then, this document is the **preferred engineering baseline**, not a fabrication authorization.
