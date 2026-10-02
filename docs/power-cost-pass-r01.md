# ENKU R0.1 power cost / quality pass

Status: **component shortlist / October 2026**

Goal: choose power components using two simultaneous filters:

1. technical quality and safety;
2. realistic small-batch cost / future margin.

Prices below are snapshot distributor references, not production quotations.

## Decision summary

### Charger / power-path: TI BQ25185 — KEEP as primary candidate

Current LCSC snapshot:
- 1+: ~USD 1.75
- 10+: ~USD 1.48
- 30+: ~USD 1.34
- 100+: ~USD 1.17

At ~3.90 PLN/USD, the IC itself is roughly:
- ~5.8 PLN at 10 units;
- ~5.2 PLN around 30 units;
- ~4.6 PLN at 100 units.

Why keep it:
- up to 1 A charging;
- true power-path;
- 4 µA battery-only quiescent current;
- battery temperature support;
- 3–18 V input.

Source:
https://www.lcsc.com/pl/product-detail/battery-management_texas-instruments-bq25185dlhr_C19725033.html

Conclusion:
The cost premium over MCP73831 is justified by eliminating a fundamental architectural weakness. Do not cost-cut back to MCP73831 for production.

### USB / Dock input selector: TPS2121 — technically excellent, cost acceptable but optional

Current LCSC snapshot:
- 10+: ~USD 0.92
- 30+: ~USD 0.79
- 100+: ~USD 0.70

Approx:
- ~3.6 PLN at 10 units;
- ~3.1 PLN around 30 units;
- ~2.7 PLN at 100 units.

Capabilities:
- dual input;
- automatic priority/source switching;
- reverse-current blocking;
- integrated FETs;
- 56 mΩ typical RDS(on);
- current limiting.

Source:
https://www.lcsc.com/product-detail/Battery-Management-ICs_Texas-Instruments-TPS2121RUXR_C485916.html

Decision:
Keep TPS2121 as the **quality baseline** for prototypes and early kits.

Before production freeze, compare it against a discrete MOSFET / ideal-diode design. Replace it only if:
- saving is meaningful across the whole board;
- reverse-current behavior stays safe;
- simultaneous USB + Dock use remains benign;
- assembly/test complexity does not erase the saving.

A ~3 PLN IC is not worth removing if the alternative adds field-failure risk.

### 3.3 V regulator: evaluate TPS63031 vs TPS62840

#### TPS63031

LCSC historical/current reference:
- 10+: ~USD 0.80
- 30+: ~USD 0.71
- 100+: ~USD 0.59

Function:
- fixed 3.3 V buck-boost;
- 1.8–5.5 V input;
- roughly 900 mA class.

Source:
https://lcsc.com/product-detail/DC-DC-Converters_TI_TPS63031DSKR_TPS63031DSKR_C15516.html

Strength:
- holds 3.3 V even when battery falls below 3.3 V;
- directly suited to a 1S battery.

Risk:
- current margin must be bench-tested against ESP32-S3 Wi-Fi + microSD + e-paper bursts.

#### TPS62840

Current LCSC snapshot:
- 10+: ~USD 1.04
- 30+: ~USD 0.94
- 100+: ~USD 0.75

Features:
- buck only;
- 1.8–6.5 V input;
- up to 750 mA;
- extremely low ~60 nA quiescent current.

Source:
https://www.lcsc.com/pl/product-detail/DC-DC-Converters_Texas-Instruments-TPS62840YBGR_C2071139.html

Decision:
TPS62840 is attractive for low sleep current but, as a buck, loses regulation headroom as a 1S Li-Po approaches the 3.3 V rail.

For ENKU, **TPS63031 is currently the more natural architecture candidate**, provided burst-current testing passes.

If 900 mA is insufficient, move up to a higher-current buck-boost such as TPS63070 rather than falling back to an inefficient always-on LDO.

### Higher-current fallback: TPS63070

Current LCSC snapshot:
- 10+: ~USD 1.15
- 30+: ~USD 0.99
- 100+: ~USD 0.87

Capabilities:
- buck-boost;
- 2–16 V input;
- substantially higher current capability;
- ~50 µA quiescent current.

Source:
https://www.lcsc.com/product-detail/C109322.html

Decision:
Do not populate by default unless bench data shows TPS63031 margin is inadequate.

### Hall sensor: DRV5032 — KEEP

Low-power versions cost approximately:
- ~USD 0.14–0.22 around 50 units depending exact variant.

Source examples:
https://www.lcsc.com/pl/product-detail/Hall-Switches_Texas-Instruments-DRV5032ZEDBZR_C266118.html
https://www.lcsc.com/pl/product-detail/C140921.html

Conclusion:
At well below 1 PLN in volume, Hall wake/sleep is not worth deleting from Cover / Pro variants.

Base may retain DNP optionality if every fraction of cost matters.

### Qi receiver: BQ51013C — Pro Wireless only

Current Mouser snapshot:
- 10+: ~USD 2.49 / around EUR 2.14 in EU listing;
- 25+: ~USD 2.28 / around EUR 1.96;
- 100+: ~USD 2.05 / around EUR 1.76.

Capabilities:
- Qi / WPC receiver;
- 5 V output;
- up to 5 W class;
- up to 1.5 A output-device rating.

Sources:
https://www.mouser.com/ProductDetail/Texas-Instruments/BQ51013CRHLR
https://www.mouser.it/c/semiconductors/power-management-ics/?series=BQ51013C

Decision:
The IC itself is not the expensive part.

The real Qi budget must include:
- coil;
- ferrite;
- tuning components;
- mechanical thickness;
- thermal validation;
- Qi interoperability / compliance work;
- yield / assembly risk.

Therefore Qi stays **Pro Wireless only**.

Do not advertise the Qi adder based solely on the receiver-IC cost.

## Recommended R0.1 power stack

```text
USB-C 5V -----\
               Dock 5V --------> TPS2121 or qualified OR-ing --> BQ25185 --> VSYS
                                                     |
Qi 5V ----------/  (Pro Wireless only)               +--> LiPo
                                                     |
                                                     +--> 3.3V buck-boost
                                                           |
                                                           +--> ESP32-S3
                                                           +--> microSD
                                                           +--> logic
```

Hard OFF disables the downstream system rails while preserving battery charging.

## Cost discipline

At 50–100 units, the premium power ICs under review are individually only a few PLN.

The correct question is not:
> "Can we remove this 3 PLN chip?"

It is:
> "Does removing it create more than 3 PLN of support, failure, assembly or redesign risk?"

For safety- or reliability-critical power blocks, ENKU should generally prefer the integrated, validated solution unless the discrete alternative saves a material percentage of total landed BOM.

## Working power-budget target per reader

Before final supplier quotes, reserve approximately:

- charger / power-path + passives: 8–12 PLN;
- external source mux / OR-ing: 3–5 PLN;
- 3.3 V buck-boost + inductor/passives: 5–8 PLN;
- USB protection / CC / filtering: 2–4 PLN;
- battery measurement / Hall / minor power control: 1–3 PLN;
- Qi receiver + coil/ferrite/tuning: additional 15–30 PLN for Pro Wireless pending actual sourcing.

Target total low-voltage power block:
- Base / Cover: ~19–32 PLN;
- Pro: ~20–34 PLN plus frontlight power;
- Pro Wireless: ~35–64 PLN including Qi allowance.

These are engineering budget envelopes, not quotes.

## Freeze criteria

Power BOM is not frozen until:
- actual battery is selected;
- charge current is thermally validated;
- 3.3 V rail survives ESP32-S3 Wi-Fi + SD write worst-case;
- USB + Dock simultaneous connection is tested;
- hard-OFF charging is verified;
- sleep leakage is measured;
- Pro frontlight current is measured;
- Qi coil/ferrite are tested in the real enclosure.
