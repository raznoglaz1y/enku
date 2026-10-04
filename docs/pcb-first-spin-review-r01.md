# ENKU R0.1 — first-spin PCB review

Status: **mandatory design-review gate before PCBWay fabrication / assembly**

Purpose: make R0.1 benefit from mistakes and revisions already absorbed by mature e-paper, ESP32 and open-hardware boards instead of treating the first ENKU PCB as an isolated experiment.

This review is intentionally stricter than "KiCad DRC passes". A board can pass DRC and still fail because of a wrong application circuit, wrong connector orientation, insufficient regulator margin, inaccessible test nodes or a footprint that cannot be assembled.

## Reference designs reviewed

The current review uses patterns and revision evidence from:

- Waveshare ESP32-S3 3.97-inch e-paper hardware and the current 3.97-inch SSD1677 application circuit;
- LilyGO T5 ePaper family;
- LilyGO EPD47 / T5 ePaper S3 revisions, including later v2.x schematic releases;
- Soldered / Inkplate hardware, including modern hierarchical KiCad projects and later hardware revisions;
- Joey Castillo's The Open Book, including multiple C2 board revisions;
- vendor reference layouts for the exact TI parts selected by ENKU;
- exact manufacturer-derived footprints for GCT USB4105, Hirose DM3AT-SF-PEJM5 and ESP32-S3-WROOM-1.

The goal is not to copy any one board. The goal is to extract decisions that survived real fabrication, assembly and iteration.

---

## 1. Do not size power from average current

### Lesson

ESP32 products fail when the regulator is selected from average reading-mode current while Wi-Fi, storage and display peaks are treated as mutually exclusive.

### ENKU action

R0.1 moved from TPS63031 to **TPS63802DLAR**.

First-spin baseline:
- 2 A-class 3.3 V buck-boost headroom;
- 0.47 µH Murata DFE201612E-R47M=P2;
- 10 µF + 100 nF input;
- 2 × 22 µF output;
- 511 kΩ / 91 kΩ feedback;
- 100 kΩ PG pull-up;
- MODE low for power-save mode;
- true hard-off through EN.

### Gate

Bench test must intentionally overlap:
- Wi-Fi TX;
- microSD write;
- EPD refresh;
- normal MCU activity.

Measure 3V3 droop, reset margin and regulator temperature near the low end of battery voltage.

---

## 2. Use the panel application circuit, not a generic controller sketch

### Lesson

An e-paper controller data sheet describes capabilities. The actual panel application circuit defines the booster values and rail network that the physical glass expects.

### ENKU action

The earlier 47 µH EPD assumption was re-reviewed against the current 3.97-inch SSD1677 circuit.

R0.1 now uses:
- TYS5040100M-10, **10 µH**;
- IRLML6346TRPBF;
- 3 × MBR0530;
- 2.2 Ω RESE sense;
- 1 MΩ GDR pull-down;
- local 4.7 µF booster input capacitor;
- dedicated VGH/VGL/VSH1/VSH2/VSL/VCOM capacitors.

The same-footprint 47 µH part remains only a laboratory alternate.

### Gate

First assembled board is not promoted until VGH, VGL, VSH1, VSH2, VSL and VCOM are scoped during refresh.

---

## 3. Exact footprints come before routing

### Lesson

Several first-spin failures are mechanical/electrical footprint failures, not logical schematic failures:
- hidden/exposed ground pads omitted;
- shell stakes missing;
- wrong FPC contact side;
- card slot facing the wrong enclosure edge;
- package copied from a "similar" part.

### ENKU action already taken

- ESP32-S3-WROOM-1 symbol/PCB now include pins **40 and 41 GND**;
- module uses the full land pattern and antenna keepout;
- GCT **USB4105-GF-A-120** exact mechanical footprint is aligned to Edge.Cuts;
- Hirose **DM3AT-SF-PEJM5** exact microSD footprint is rotated so the card exits the intended enclosure edge;
- dual-contact Hirose EPD/frontlight FPC strategy removes contact-side ambiguity.

### Hard blockers still requiring exact-footprint sign-off

Before routing/fabrication:
- TPS2121 RUX package;
- BQ25185 DLH package;
- BMI270 LGA;
- TPS923610 DRL;
- EPD 24-pin Hirose connector;
- frontlight 6-pin Hirose connector;
- final battery connector;
- final tact/page switch;
- final dock pogo geometry.

No placement-only placeholder may survive the fabrication tag.

---

## 4. Antenna keepout is a volume, not a silkscreen rectangle

### Lesson

ESP32 module antennas lose margin when copper, screws, batteries, flex cables or enclosure metal violate the antenna zone even if the module itself is "at the edge".

### ENKU action

- U1 antenna points toward the left board edge;
- U1 is rotated 90 degrees so the module antenna keepout projects beyond the left board edge;
- H1 was moved because the original mounting-hole position intruded into the real keepout corridor;
- all copper layers must respect the module keepout;
- battery, EPD flex and Qi ferrite must be checked in the 3D assembly, not only PCB view.

### Gate

Antenna keepout is checked in:
1. KiCad copper layers;
2. STEP/3D assembly;
3. final enclosure with battery/display/flex/Qi stack.

---

## 5. Use comfortable fabrication rules unless density forces otherwise

### Revision evidence

The Open Book C2 revisions show a move toward more assembly-friendly geometry in places:
- a relevant resistor was changed from 0402 to 0603;
- board rules moved from 5 mil to 6 mil.

The broader lesson is not "never use 0402". It is: do not spend fabrication margin where it buys nothing.

### ENKU rule

Default for R0.1:
- 0603 resistors/capacitors where area allows;
- 0805 where voltage derating, capacitance, power or probing benefits;
- 0402 only for genuinely dense remap/tuning fields or package-imposed placement;
- avoid minimum PCBWay geometry as a design target;
- use wider power traces/planes and practical via sizes.

Routing rules are frozen only after the actual PCBWay stackup/impedance quote is selected.

---

## 5A. Charger island follows the vendor layout, not ratsnest convenience

The BQ25185 placement/routing must be reviewed as one functional island before individual unrouted nets are optimized.

R0.1 rules:
- IN, SYS and BAT capacitors stay at their respective IC pins with short ground return;
- ISET and ILIM/VSET programming resistors stay close to the charger, following TI's layout example rather than being routed across the power island; use 1% parts for programmed-current/voltage accuracy;
- STAT1/STAT2 are open-drain status outputs: keep their pull-ups outside the high-current core and route them as low-current status signals;
- high-current IN/SYS/BAT paths remain wide and direct;
- charger status/control lines leave the island only after the local power/current-setting geometry is solved;
- the exposed/thermal ground region gets a low-impedance ground connection and nearby stitching;
- do not route USB, ADC, IMU or other sensitive signals through the charger current paths;
- if existing tracks prevent the vendor-style component placement, rework the island as a unit rather than adding long vias/tracks around it.

Evidence source: TI BQ25185 datasheet layout guidance and board-layout example, reviewed 2026-10-04.

---

## 6. Switching regulators get a physical hot-loop review

### Lesson

Correct values do not rescue a bad switching layout.

### ENKU TPS63802 layout review

- input cap directly at VIN/GND;
- output caps directly at VOUT/GND;
- inductor immediately at L1/L2;
- L1/L2 copper short and small;
- FB divider on the quiet side;
- REG_FB kept away from L1/L2 and inductor copper;
- AGND/control return joins locally to power ground;
- no sensitive BAT_ADC / IMU / USB data path through the hot loop.

### ENKU EPD/frontlight review

The same rule applies to:
- SSD1677 boost/negative-pump island;
- TPS923610 frontlight boost.

Keep those loops compact and away from ESP32 antenna, IMU and ADC sense.

---

## 7. USB-C needs electrical and mechanical first-line protection

### ENKU baseline

- full 16-pin USB2 receptacle;
- both CC pins have 5.1 kΩ Rd;
- D+/D- ESD device is placed close to the connector;
- D+/D- keep 0 Ω tuning footprints that can become 22 Ω if bench/SI work calls for it;
- shell has an RC-to-ground strategy plus optional 0 Ω population;
- no USB-PD dependency;
- VBUS feeds the source selector/charger, never the 3V3 rail directly.

### Gate

Check:
- connector shell/stake drill geometry;
- board-edge alignment;
- enclosure plug clearance;
- D+/D- continuity and swap;
- USB enumeration before any other peripheral is blamed.

---

## 8. microSD deserves its own bring-up plan

### Lesson

Mature boards treat storage as a noisy/current-spiky peripheral, not just four SPI wires. Inkplate explicitly distinguishes a microSD supply domain.

### ENKU R0.1 baseline

- exact Hirose socket;
- SPI mode;
- independent CS;
- CS pull-up;
- local 10 µF + 100 nF;
- 0 Ω / 22 Ω tuning sites on clock/data;
- physical card-eject direction verified before routing.

### First-spin decision

Keep direct 3V3_SYS power for the lowest-risk first assembly, but reserve a future/optional switched `3V3_MICROSD` path if sleep-current measurement shows the card dominates standby.

Do not add a load switch blindly before measuring the selected card behavior.

---

## 9. Test access is part of the circuit

### Mandatory R0.1 measurement nodes

At minimum expose probe-friendly pads for:
- GND;
- VBUS_USB;
- VBUS_DOCK;
- VBAT;
- VSYS;
- 3V3_SYS;
- SYS_EN;
- REG_PG;
- EPD_GDR;
- EPD_RESE;
- EPD_VGH;
- EPD_VGL;
- EPD_VCOM;
- FL_LED_PLUS;
- FL_FB.

These are not decorative test points. They define the first-board bring-up sequence and make a failed board diagnosable without scraping solder mask.

Where possible, put prototype measurement pads on the accessible rear side and outside the Qi keepout.

---

## 10. Unknown first-article pinouts get reversible population options

### Lesson

A zero-ohm option costs less than a respin when an FPC or optional accessory has a vendor-document ambiguity.

### ENKU uses this deliberately

- 18-position frontlight raw-pin remap;
- source-mux population straps;
- D+/D- tuning links;
- DNP Qi path;
- same-footprint EPD inductor alternate.

### Rule

DNP matrices are allowed only where there is a concrete first-article uncertainty. They are not a substitute for finishing the schematic.

---

## 11. Separate noisy, sensitive and RF zones

R0.1 physical zones must remain recognizable after routing:

- left edge: ESP32 + antenna keepout;
- top: EPD FPC + EPD HV;
- upper-right: Pro frontlight;
- lower: USB / source mux / charger / 3V3 converter / battery;
- lower-left edge: microSD;
- right edge: buttons;
- center: Pro Wireless Qi keepout;
- quiet area: BMI270 and BAT_ADC.

Do not route:
- switching nodes under/through IMU;
- EPD HV near antenna;
- USB differential path through switching island;
- BAT_ADC parallel to inductor/switch nodes;
- copper through the Qi coil/ferrite exclusion zone.

---

## 12. Do not optimize the PCB for assembly price before it works

### First-spin priorities

1. electrical correctness;
2. exact footprint/mechanics;
3. probe access;
4. assembly yield;
5. RF/power margin;
6. only then BOM/area cost-down.

A few extra 0603s, test pads and DNP options are cheaper than a second failed PCBA batch.

Cost-down belongs after first hardware measurements.

---

## Current layout checkpoint — 04 Oct 2026

R0.1 has crossed from placement/net-sync into active routing.

Current repository checkpoint:
- native KiCad ERC: **0 violations**;
- native schematic netlist: **152 / 152 components**;
- schematic ↔ PCB parity: **0 issues**;
- first-spin critical copper gate: **0 clearance / 0 shorting blockers**;
- 128 routed copper segments and 30 vias at the parity-closure checkpoint;
- all previously missing PCB footprints are now instantiated, including debug and optional Qi interfaces;
- locked custom footprints now exist for microSD and provisional physical controls;
- top and bottom GND pours are defined and pass the current critical DRC gate;
- the remaining closure is now a real routing backlog rather than schematic/PCB synchronization debt.

This does **not** mean the board is fabrication-ready. The release gate remains zero unexplained DRC, zero unrouted connections, zone refill/recheck, footprint/orientation audit and reproducible fabrication outputs.

## 13. PCBWay fabrication/assembly package must be reproducible

The fabrication tag must contain or generate:

- KiCad source;
- Gerbers;
- NC drill;
- board stackup / fab notes;
- BOM with exact MPN and population variant;
- CPL / centroid / pick-and-place with X/Y, side and rotations verified;
- assembly drawings for both sides;
- DNP list / Base-Pro-Pro Wireless matrix;
- critical-component orientation/polarity drawing, especially IC pin 1, diodes and connectors;
- STEP model;
- schematic PDF;
- bring-up checklist;
- test-point map;
- known-first-article notes;
- SHA / release tag tying all outputs to one source revision.

Never send PCBWay a hand-edited BOM/CPL whose source does not match the tagged KiCad project.

---

## 14. Before-fabrication hard gate

R0.1 may be released to PCBWay only when all are true:

- [ ] all schematic sheets open in KiCad;
- [ ] KiCad ERC reviewed to zero unexplained errors;
- [ ] all PCB footprints are exact or explicitly manufacturer-verified; core IC/FPC set is verified, enclosure-dependent buttons/battery/dock remain open;
- [ ] no placement placeholder remains;
- [ ] schematic ↔ PCB pad numbers/nets are synchronized;
- [ ] antenna keepout clear on every copper layer and in the 3D stack;
- [ ] FPC insertion direction / latch access verified;
- [ ] microSD insertion/ejection clearance verified;
- [ ] USB-C plug and shell clearance verified;
- [ ] battery connector polarity physically verified;
- [ ] TPS63802 hot loop reviewed;
- [ ] EPD HV hot loop reviewed;
- [ ] TPS923610 hot loop reviewed;
- [ ] Qi keepout clear on every layer/variant;
- [ ] all mandatory test pads placed and accessible;
- [ ] 4-layer stackup and routing rules frozen to PCBWay capability;
- [ ] DRC reviewed to zero unexplained errors;
- [ ] no unrouted nets;
- [ ] zones refilled and re-DRC'd;
- [ ] silkscreen not under solderable pads / connector contacts;
- [ ] BOM exact MPN / package / value audit complete;
- [ ] CPL rotation audit complete;
- [ ] 3D enclosure interference review complete;
- [ ] Gerber/drill visual review complete;
- [ ] independent final schematic/PCB review complete.

---

## 15. Serviceability and repairability gate

R0.1 must be diagnosable and repairable with normal bench tools. Passing DRC alone is not sufficient.

### Layout rules

- keep 0603/0805 passives as the default where electrical density does not force smaller packages;
- do not bury first-line protection, tuning links or configuration straps under connectors, the ESP32 module, display flexes or the battery;
- preserve visible reference designators for service-critical parts where silkscreen space allows;
- leave soldering-iron / hot-air access around USB protection, charger, source mux, 3V3 regulator, EPD power and frontlight power components;
- keep test points outside connector latch travel, enclosure bosses, Qi exclusion and battery adhesive zones;
- avoid placing two unrelated critical parts so close that replacing one requires removing the other;
- every power domain must have an accessible GND reference and at least one accessible measurement point;
- tuning / recovery parts (0R links, pull-ups, pull-downs, DNP options) must remain individually replaceable;
- prefer ordinary stocked package families and exact MPNs that have realistic second-source or replacement availability where the function permits it.

### Diagnostic partition

A dead board should be separable on the bench into:
1. USB / dock input;
2. source mux;
3. charger / battery;
4. VSYS;
5. 3V3 buck-boost;
6. ESP32 / programming;
7. storage and sensors;
8. EPD logic / HV;
9. frontlight;
10. optional Qi.

No downstream fault should require destructive probing to determine which of these domains failed.

### Fabrication/assembly rule

Do not accept a routing shortcut that:
- creates an inaccessible repair joint;
- routes a sensitive sense node through a switching-current corridor;
- removes useful probing access;
- depends on PCBWay minimum geometry without a concrete density need;
- requires a broad DRC exception instead of a local, documented manufacturer-geometry exception.

### Release evidence

Before the fabrication tag, publish:
- annotated top/bottom service map;
- test-point table with expected idle/active voltages;
- replaceable critical-component list and package;
- DNP / tuning matrix;
- bring-up fault tree for the five power checkpoints: VBUS, VBAT, VSYS, 3V3_SYS and EPD HV.

---

## 16. First-board bring-up order

Do not plug the display and "see what happens".

1. visual inspection + resistance-to-ground check;
2. current-limited USB input, no battery;
3. verify source mux output;
4. verify BQ25185 SYS / charging behavior;
5. verify TPS63802 3V3 and PG;
6. flash minimal ESP32 firmware;
7. verify USB and UART;
8. verify buttons / I2C IMU / Hall;
9. verify microSD;
10. attach EPD and scope HV rails before a full refresh;
11. validate EPD refresh;
12. populate/validate Pro frontlight remap and current;
13. test hard-off while charging;
14. test simultaneous Wi-Fi + SD + EPD peak load;
15. measure sleep/off leakage;
16. only then test Qi variant.

If a stage fails, stop at that power domain instead of adding more connected hardware.

---

## Current ENKU design changes caused by this review

Already applied:
- exact ESP32 41-pad model and antenna keepout;
- exact USB-C and microSD mechanics;
- microSD orientation corrected;
- TPS63031 rejected for first-spin margin;
- TPS63802 selected and placed;
- EPD L changed from 47 µH assumption to current 10 µH application-circuit baseline;
- 1 MΩ GDR pull-down restored;
- EPD local input capacitor restored;
- hardware structural/placement gates added to CI;
- false CI orientation rule found and fixed.

Still open before routing:
- exact enclosure-dependent button/battery/dock footprints; core TPS2121/BQ25185/BMI270/TPS923610/Hirose footprints are now verified;
- mandatory test-point placement;
- native KiCad ERC;
- final PCBWay stackup/rules;
- final net synchronization for every component.
