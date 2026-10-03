#!/usr/bin/env python3
from pathlib import Path
import re
import sys

BASE = Path(__file__).resolve().parent
PCB = BASE / "enku-mainboard-r0.1.kicad_pcb"
SHEETS = (
    "power.kicad_sch",
    "mcu_io.kicad_sch",
    "epd_hv.kicad_sch",
    "frontlight.kicad_sch",
    "connectors.kicad_sch",
)

# Placement debt discovered before routing on 2026-10-03.
# This is a ceiling, not an acceptance list: refs may disappear from this set only by
# being placed on PCB. Any NEW missing ref is an immediate CI failure.
KNOWN_UNPLACED = {
    "C0_EPD_VCI","C1_EPD_VDD","C5_EPD_VSH1","C6_EPD_VSH2","C7_EPD_VSL","C8_EPD_VCOM",
    "C_BAT_ADC","C_CHG_BAT","C_CHG_IN","C_CHG_IN_HF","C_DOCK_DET","C_DOCK_IN","C_EN",
    "C_ESP_HF","C_FL_IN","C_FL_IN_HF","C_HALL","C_IMU_VDD","C_IMU_VDDIO","C_MUX_OUT",
    "C_MUX_SS","C_SD_BULK","C_SD_HF","C_USB_IN","C_USB_SHIELD","J_DBG","J_QI",
    "R_BAT_BOT","R_BAT_TOP","R_CC1","R_CC2","R_CHG_CE_PD","R_CHG_ILIM_VSET","R_CHG_ISET",
    "R_CP2_DOCK","R_CP2_GND","R_DOCK_DET_PD","R_DOCK_DET_SER","R_EPD_BS1","R_EPD_CS",
    "R_EPD_CS_PU","R_EPD_DC","R_EPD_MOSI","R_EPD_RST","R_EPD_RST_PU","R_EPD_SCLK",
    "R_EPD_VCI_LINK","R_FL_ADIM_LINK","R_FL_ADIM_PD","R_FL_COOL_PD","R_FL_WARM_PD",
    "R_I2C_SCL_PU","R_I2C_SDA_PU","R_MUX_ILIM","R_OV1_GND","R_OV2_GND","R_PR1_GND",
    "R_PR1_USB","R_SD_CS","R_SD_CS_PU","R_SD_MISO","R_SD_MOSI","R_SD_SCLK","R_STAT1_PU",
    "R_STAT2_PU","R_SYS_EN_PD","R_USB_DM","R_USB_DP","R_USB_SHIELD","R_USB_SHIELD_0R",
    "SW_BOOT",
}


def schematic_refs() -> set[str]:
    refs: set[str] = set()
    for sheet in SHEETS:
        text = (BASE / sheet).read_text(encoding="utf-8")
        refs.update(
            r for r in re.findall(r'\(reference "([^"]+)"\)', text)
            if r and not r.startswith("#")
        )
    return refs


def pcb_refs() -> set[str]:
    text = PCB.read_text(encoding="utf-8")
    return {
        r for r in re.findall(r'\(property "Reference" "([^"]+)"', text)
        if r and r != "REF**"
    }


def main() -> int:
    sch = schematic_refs()
    pcb = pcb_refs()
    missing = sch - pcb
    unexpected = missing - KNOWN_UNPLACED
    retired = KNOWN_UNPLACED - missing

    errors: list[str] = []
    if unexpected:
        errors.append(
            "new schematic refs missing from PCB: " + ", ".join(sorted(unexpected))
        )

    # Mounting holes are board-only mechanics and are intentionally not schematic refs.
    board_only = pcb - sch - {"H1", "H2", "H3", "H4"}
    if board_only:
        errors.append(
            "unexpected PCB-only references: " + ", ".join(sorted(board_only))
        )

    if errors:
        for err in errors:
            print(f"ERROR: {err}")
        print(f"ENKU PCB population gate: FAIL ({len(errors)} issue(s))")
        return 1

    print("ENKU PCB population gate: PASS")
    print(f"Schematic refs: {len(sch)}")
    print(f"PCB refs: {len(pcb)}")
    print(f"Known refs still awaiting placement: {len(missing)}")
    if missing:
        print("UNPLACED: " + ", ".join(sorted(missing)))
    if retired:
        print(f"Placement progress since baseline: {len(retired)} ref(s)")
        print("PLACED_FROM_DEBT: " + ", ".join(sorted(retired)))
    if not missing:
        print("PCB population complete: zero schematic refs missing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
