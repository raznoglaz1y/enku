# ENKU Mainboard R0.1 — schematic net / block specification

Status: **schematic-capture baseline / EPD-HV exact parts still blocked**

Purpose: define the actual electrical structure that the KiCad schematic must implement before placement/routing.

## 1. Power input tree

Named nets:

- `VBUS_USB`
- `VBUS_DOCK`
- `VBUS_EXT`
- `VBAT`
- `VSYS`
- `3V3_SYS`

Flow:

```text
USB-C VBUS -> USB protection -> TPS2121 IN1
Dock +5V   -> Dock protection -> TPS2121 IN2
TPS2121 OUT -> VBUS_EXT -> BQ25185 IN
BQ25185 BAT -> VBAT
BQ25185 SYS -> VSYS
VSYS -> TPS63031 -> 3V3_SYS
```

R0.1 hard power:
- physical switch controls TPS63031 EN;
- charger remains active when 3V3_SYS is off;
- EPD-HV and frontlight must not remain powered when system-off.

## 2. USB-C

References:

- J1 — USB-C receptacle
- U_USB_ESD — USB ESD device
- R_CC1 / R_CC2 — 5.1 kΩ
- optional R_USB_DP / R_USB_DM series footprints

Signals:

- `USB_DP` -> ESP32 GPIO20
- `USB_DM` -> ESP32 GPIO19
- `VBUS_USB`
- `GND`

No USB-PD requirement for R0.1.

## 3. Dock

Board-side exposed contacts:

1. GND
2. +5V
3. DOCK_DETECT
4. GND

Nets:

- `VBUS_DOCK`
- `DOCK_DETECT`
- `GND`

DOCK_DETECT -> GPIO2 through a defined pull/protection network.

Dock power and USB power must not backfeed each other.

## 4. Battery / charge

Reference:
- U_CHG — BQ25185

Nets:
- `VBUS_EXT`
- `VBAT`
- `VSYS`
- `CHG_STAT1`
- `CHG_STAT2`
- `BAT_TS`

Prototype starting point:
- 4.2 V battery regulation;
- 500 mA input-current limit;
- 300 mA initial fast-charge target.

Production battery temperature protection must use the selected battery/NTC solution.

## 5. 3.3 V system regulator

Reference:
- U_3V3 — TPS63031 candidate

Nets:
- IN: `VSYS`
- OUT: `3V3_SYS`
- EN: `SYS_EN`

Hard power switch drives `SYS_EN`.

Add:
- 1.5 µH candidate inductor per reference topology;
- input/output capacitors per TI reference;
- test point on VSYS and 3V3_SYS.

Fallback:
- TPS63070 if burst-current validation fails.

## 6. ESP32-S3

Reference:
- U1 — ESP32-S3-WROOM-1-N16R8

Power:
- 3V3_SYS
- GND

Core pin allocation:
- GPIO19 USB D-
- GPIO20 USB D+
- GPIO12 SPI_SCLK
- GPIO11 SPI_MOSI
- GPIO13 SPI_MISO
- GPIO10 SD_CS
- GPIO14 EPD_CS
- GPIO21 EPD_DC
- GPIO47 EPD_RST
- GPIO48 EPD_BUSY
- GPIO4 BTN_PREV
- GPIO5 BTN_NEXT
- GPIO6 BTN_SELECT
- GPIO7 BTN_BACK
- GPIO8 I2C_SDA
- GPIO9 I2C_SCL
- GPIO18 IMU_INT1
- GPIO17 HALL_INT
- GPIO1 BAT_ADC
- GPIO2 DOCK_DETECT
- GPIO15 CHG_STATUS
- GPIO16 QI_STATUS
- GPIO39 FL_WARM_PWM
- GPIO40 FL_COOL_PWM
- GPIO41 FL_ENABLE
- GPIO43 UART_TX
- GPIO44 UART_RX
- GPIO0 BOOT

Do not load strapping pins GPIO0/3/45/46 in a way that changes boot state.

## 7. microSD

Reference:
- J_SD — microSD connector

Mode:
- SPI for R0.1 common-mainboard path.

Signals:
- SPI_SCLK
- SPI_MOSI
- SPI_MISO
- SD_CS

Add:
- local 3V3 decoupling;
- optional series resistor footprints if signal integrity requires tuning;
- card-detect only if the selected connector provides it at acceptable cost.

## 8. EPD logic connector

Reference:
- J_EPD — 24-pin 0.5 mm FPC

Logical nets:
- EPD_CS
- EPD_DC
- EPD_RST
- EPD_BUSY
- SPI_SCLK
- SPI_MOSI
- EPD_GDR
- EPD_RESE
- EPD_VSH1
- EPD_VSH2
- EPD_VGH
- EPD_VGL
- EPD_VSL
- EPD_VCOM
- EPD_VDDIO
- EPD_VCI
- EPD_VDD
- EPD_VPP
- GND

BS1 is intended LOW for 4-wire SPI.

Exact connector pin numbers remain blocked until current GDEY drawings are directly verified.

## 9. EPD-HV block

This block must be drawn as a dedicated schematic sheet.

Named nets:
- EPD_GDR
- EPD_RESE
- EPD_VGH
- EPD_VGL
- EPD_VSH1
- EPD_VSH2
- EPD_VSL
- EPD_VCOM

Required classes of components:
- external N-MOSFET;
- current-sense resistor;
- inductor;
- Schottky network;
- 50 V-class HV capacitors;
- local logic/core decoupling.

Current provisional values may be shown in notes but must carry a `VERIFY-GOOD-DISPLAY` marker.

No PCBWay order while any EPD-HV critical designator is still generic/TBD.

## 10. BMI270

Reference:
- U_IMU — BMI270

Signals:
- I2C_SDA
- I2C_SCL
- IMU_INT1

Power:
- 3V3_SYS

Required:
- VDD/VDDIO decoupling per Bosch reference;
- I²C pull-ups sized for the final bus;
- orientation marker in schematic and PCB notes.

## 11. Hall sensor

Reference:
- U_HALL — DRV5032-family candidate

Signals:
- HALL_INT -> GPIO17

Power:
- select the exact low-power supply configuration after part-variant freeze.

Placement is mechanically tied to Cover magnet geometry.

## 12. Physical controls

References:
- SW_PREV
- SW_NEXT
- SW_SELECT
- SW_BACK
- SW_BOOT
- SW_POWER

Direct GPIO buttons for PREV/NEXT/SELECT/BACK.

No ADC key ladder in R0.1.

SW_POWER controls SYS_EN, not firmware.

## 13. Battery ADC

Net:
- BAT_ADC -> GPIO1

Use a high-value divider sized to minimize standby drain.

Add:
- RC filter;
- ADC protection margin;
- optional switched divider only if power measurements justify the added complexity.

## 14. Pro frontlight

Separate schematic sheet / population option.

References:
- J_FL — 6-pin 0.5 mm FPC
- U_FL_WARM — prototype current driver candidate
- U_FL_COOL — prototype current driver candidate

Signals:
- FL_WARM_PWM
- FL_COOL_PWM
- FL_ENABLE

Power source:
- final choice must avoid bypassing hard-off.

Driver remains prototype candidate until the current FL0426-S01C pin assignment is verified.

## 15. Pro Wireless

Separate DNP population block.

Reference:
- U_QI — BQ51013C-family candidate

Nets:
- `QI_5V`
- `QI_STATUS`

QI_5V must enter the same protected charging architecture rather than charge the cell independently.

Three-source arbitration remains a sub-block to freeze after USB/Dock/Qi interaction is tested.

## 16. Debug / test

Expose:
- UART TX/RX
- BOOT
- EN/reset service access
- GND
- 3V3_SYS
- VSYS
- VBAT
- VBUS_USB
- VBUS_DOCK
- EPD BUSY/RST
- EPD HV rails where safely probeable

Add current-measurement 0 Ω links where useful:
- VSYS -> U_3V3 input
- optional EPD-HV supply input
- optional frontlight supply input

## 17. PCBWay schematic gate

The schematic can move to PCB placement only when:

- every non-DNP component has an exact MPN or an explicitly approved generic passive specification;
- all connector contact orientations are verified;
- EPD-HV exact parts are frozen;
- no unresolved strapping-pin conflict remains;
- ERC passes;
- variant DNP matrix is generated from the schematic/BOM;
- Base, Pro and Pro Wireless can be built from the intended common PCB without bodge wires.
