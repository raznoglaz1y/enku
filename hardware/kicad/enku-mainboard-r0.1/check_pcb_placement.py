#!/usr/bin/env python3
"""Structural/mechanical gate for ENKU Mainboard R0.1 placement baseline."""

from __future__ import annotations

from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
PCB = HERE / "enku-mainboard-r0.1.kicad_pcb"

REQUIRED_REFS = {
    "U1", "U_IMU", "U_HALL",
    "J_EPD", "J_FL", "J_SD", "J_USB", "J_DOCK", "J_BAT",
    "U_SRC", "U_CHG", "U_3V3", "U_FL", "U_USB_ESD",
    "L_EPD", "Q_EPD", "D1_EPD", "D2_EPD", "D3_EPD", "R_RESE",
    "L_FL", "R_FL_SET", "Q_FL_WARM", "Q_FL_COOL",
    "L_3V3", "R_3V3_FB_TOP", "R_3V3_FB_BOT", "R_3V3_PG_PU", "C_3V3_IN", "C_3V3_IN_HF", "C_3V3_OUT1", "C_3V3_OUT2", "SW_POWER",
    "SW_PREV", "SW_NEXT", "SW_SELECT", "SW_BACK",
    "TP_GND_PWR", "TP_VBUS_USB", "TP_VBUS_DOCK", "TP_VBAT", "TP_VSYS", "TP_3V3", "TP_SYS_EN", "TP_REG_PG",
    "TP_EPD_GDR", "TP_EPD_RESE", "TP_EPD_VGH", "TP_EPD_VGL", "TP_EPD_VCOM", "TP_GND_EPD", "TP_FL_LED_PLUS", "TP_FL_FB",
}

BOARD = (20.0, 20.0, 74.0, 114.0)
EXPECTED_SIZE = (54.0, 94.0)

# Placement-only coordinate gates. These are broad regions, not final courtyard DRC.
REGIONS = {
    "U1": (20.0, 31.0, 46.0, 59.0),
    "J_EPD": (48.0, 24.0, 72.0, 32.0),
    "J_FL": (62.0, 31.0, 72.0, 37.0),
    "J_SD": (20.0, 94.0, 38.0, 113.0),
    "J_USB": (40.0, 105.0, 54.0, 114.0),
    "J_DOCK": (38.0, 95.0, 56.0, 104.0),
    "SW_POWER": (23.0, 25.0, 38.0, 33.0),
    "SW_PREV": (68.5, 48.0, 74.0, 58.0),
    "SW_NEXT": (68.5, 56.0, 74.0, 66.0),
    "SW_SELECT": (68.5, 64.0, 74.0, 74.0),
    "SW_BACK": (68.5, 72.0, 74.0, 82.0),
    "U_SRC": (39.0, 87.0, 47.0, 96.0),
    "U_CHG": (45.0, 87.0, 52.0, 96.0),
    "U_3V3": (50.0, 87.0, 58.0, 96.0),
}

def balanced(text: str) -> tuple[bool, str]:
    depth = 0
    in_string = False
    escaped = False
    for i, ch in enumerate(text):
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth < 0:
                return False, f"unexpected ')' at byte {i}"
    if in_string:
        return False, "unterminated string"
    if depth:
        return False, f"parenthesis depth ends at {depth}"
    return True, "ok"

def footprint_blocks(text: str):
    """Yield balanced top-level footprint blocks without a full KiCad parser."""
    pos = 0
    while True:
        start = text.find("(footprint ", pos)
        if start < 0:
            return
        depth = 0
        in_string = False
        escaped = False
        end = None
        for i in range(start, len(text)):
            ch = text[i]
            if in_string:
                if escaped:
                    escaped = False
                elif ch == "\\":
                    escaped = True
                elif ch == '"':
                    in_string = False
                continue
            if ch == '"':
                in_string = True
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    end = i + 1
                    break
        if end is None:
            return
        yield text[start:end]
        pos = end

def parse_ref_and_at(block: str):
    ref_m = re.search(r'\(property "Reference" "([^"]+)"', block)
    at_m = re.search(r'\(at\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)(?:\s+(-?\d+(?:\.\d+)?))?\)', block)
    if not ref_m or not at_m:
        return None
    return ref_m.group(1), float(at_m.group(1)), float(at_m.group(2)), float(at_m.group(3) or 0)

def main() -> int:
    errors: list[str] = []
    if not PCB.is_file():
        print(f"ERROR: missing {PCB.name}")
        return 1
    text = PCB.read_text(encoding="utf-8")

    ok, why = balanced(text)
    if not ok:
        errors.append(f"malformed PCB S-expression: {why}")

    outline = re.search(
        r'\(gr_rect \(start 20(?:\.0+)? 20(?:\.0+)?\) \(end 74(?:\.0+)? 114(?:\.0+)?\)',
        text
    )
    if not outline:
        errors.append("54x94 mm R0.1 board outline is missing or changed")

    refs: dict[str, tuple[float,float,float]] = {}
    duplicates: set[str] = set()
    for block in footprint_blocks(text):
        parsed = parse_ref_and_at(block)
        if not parsed:
            continue
        ref, x, y, rot = parsed
        if ref in refs:
            duplicates.add(ref)
        refs[ref] = (x, y, rot)

    for ref in sorted(REQUIRED_REFS):
        if ref not in refs:
            errors.append(f"missing critical placed reference {ref}")

    for ref in sorted(duplicates):
        errors.append(f"duplicate PCB reference {ref}")

    x0, y0, x1, y1 = BOARD
    for ref, (x,y,rot) in refs.items():
        if ref.startswith("H"):
            continue
        if not (x0 <= x <= x1 and y0 <= y <= y1):
            errors.append(f"{ref} origin outside board: ({x}, {y})")

    for ref, region in REGIONS.items():
        if ref not in refs:
            continue
        x, y, _ = refs[ref]
        rx0, ry0, rx1, ry1 = region
        if not (rx0 <= x <= rx1 and ry0 <= y <= ry1):
            errors.append(
                f"{ref} escaped placement region: ({x:.2f},{y:.2f}) "
                f"not in [{rx0},{ry0}]..[{rx1},{ry1}]"
            )

    # Critical mechanical invariants.
    if "U1" in refs:
        x, y, rot = refs["U1"]
        if abs(rot - 90.0) > 0.1:
            errors.append("U1 must stay rotated 90 deg with antenna keepout toward left edge")
    for ref in ("SW_PREV","SW_NEXT","SW_SELECT","SW_BACK"):
        if ref in refs and refs[ref][0] < 68.5:
            errors.append(f"{ref} left the right-side thumb rail")

    # WROOM body exclusion. The exact module body occupies approximately
    # x=20.25..45.75, y=36..54 at U1=(33,45), rotation 90 deg.
    # Catch footprints whose origins drift underneath the soldered module.
    for block in footprint_blocks(text):
        parsed = parse_ref_and_at(block)
        if not parsed:
            continue
        ref, x, y, _ = parsed
        if ref == "U1" or ref.startswith("H"):
            continue
        if '(layer "F.Cu")' in block and 20.0 < x < 46.0 and 35.5 < y < 54.5:
            errors.append(f"{ref} origin is under ESP32-S3-WROOM-1 module body")

    # Qi keepout remains route-free in Baseline B. A simple placement-origin gate
    # catches large ICs accidentally dropped into the reserved center.
    for ref, (x,y,_) in refs.items():
        if ref.startswith(("H","SW_")) or ref in {"U1"}:
            continue
        if 31.0 < x < 63.0 and 54.0 < y < 86.0:
            errors.append(f"{ref} origin is inside Pro Wireless Qi keepout")

    # Baseline B intentionally has no copper routing yet.
    if re.search(r'\n\s*\(segment\s', text):
        errors.append("copper segment found before schematic-to-PCB net synchronization gate")
    if re.search(r'\n\s*\(via\s', text):
        errors.append("via found before schematic-to-PCB net synchronization gate")

    if "PLACEMENT BASELINE C" not in text:
        errors.append("placement baseline C banner missing")

    tp_refs = [r for r in REQUIRED_REFS if r.startswith("TP_")]
    if len(tp_refs) < 16:
        errors.append(f"expected at least 16 mandatory bring-up test points, gate has {len(tp_refs)}")
    for ref in tp_refs:
        marker = f'(property "Reference" "{ref}"'
        p = text.find(marker)
        if p < 0:
            continue
        fp_start = text.rfind("(footprint ", 0, p)
        fp_end = text.find("\n  )", p)
        block = text[fp_start:fp_end] if fp_start >= 0 and fp_end > p else ""
        if '(layer "B.Cu")' not in block:
            errors.append(f"{ref} must remain on accessible rear copper")
        pad_line = next((ln for ln in block.splitlines() if '(pad "1"' in ln), "")
        if '"B.Paste"' in pad_line:
            errors.append(f"{ref} probe pad must not have solder paste")

    forbidden_resolved_placeholders = (
        "R0.1_FH34SRJ_24_DUAL_CONTACT_PLACEHOLDER",
        "R0.1_FH34SRJ_6_DUAL_CONTACT_PLACEHOLDER",
        "R0.1_TPS923610_SOT563_PLACEHOLDER",
        "TPS2121_PLACEMENT_PLACEHOLDER",
        "BQ25185DLHR_PLACEMENT_PLACEHOLDER",
        "R0.1_TYS5040_47uH_PLACEHOLDER",
    )
    for token in forbidden_resolved_placeholders:
        if token in text:
            errors.append(f"resolved footprint placeholder returned: {token}")

    exact_footprint_tokens = (
        "ENKU:TPS2121_RUX0012A",
        "ENKU:BQ25185_DLH0010A",
        "ENKU:BMI270_Bosch_LGA14",
        "ENKU:TPS923610_DRL0006A",
        "ENKU:FH34SRJ-24S-0.5SH",
        "ENKU:FH34SRJ-6S-0.5SH",
        "ENKU:TYS5040_5x5",
    )
    for token in exact_footprint_tokens:
        if token not in text:
            errors.append(f"exact first-spin footprint missing: {token}")

    # Exact pad-number gates: catch visually plausible but electrically impossible placeholders.
    blocks_by_ref: dict[str, str] = {}
    for block in footprint_blocks(text):
        parsed = parse_ref_and_at(block)
        if parsed:
            blocks_by_ref[parsed[0]] = block

    expected_pads = {
        "U_SRC": {str(i) for i in range(1, 13)},
        "U_CHG": {str(i) for i in range(1, 12)},
        "U_IMU": {str(i) for i in range(1, 15)},
        "U_FL": {str(i) for i in range(1, 7)},
        "J_EPD": {*(str(i) for i in range(1, 25)), "S1", "S2"},
        "J_FL": {*(str(i) for i in range(1, 7)), "S1", "S2"},
    }
    for ref, expected in expected_pads.items():
        block = blocks_by_ref.get(ref, "")
        actual = set(re.findall(r'\(pad "([^"]+)"', block))
        missing = expected - actual
        if missing:
            errors.append(f"{ref} missing exact pads: {sorted(missing)}")

    # Electrical package invariants.
    chg_block = blocks_by_ref.get("U_CHG", "")
    if not re.search(r'\(pad "11"[^\n]*\(net \d+ "GND"\)', chg_block):
        errors.append("BQ25185 exposed pad 11 must be tied to GND")
    for ref in ("J_EPD", "J_FL"):
        block = blocks_by_ref.get(ref, "")
        for shield in ("S1", "S2"):
            if not re.search(rf'\(pad "{shield}"[^\n]*\(net \d+ "GND"\)', block):
                errors.append(f"{ref} {shield} retention tab must be tied to GND")

    # Reproducible local footprint library is part of the release source.
    fp_table = HERE / "fp-lib-table"
    if not fp_table.is_file() or "ENKU.pretty" not in fp_table.read_text(encoding="utf-8"):
        errors.append("local ENKU.pretty library is not registered in fp-lib-table")
    local_footprints = (
        "TPS2121_RUX0012A.kicad_mod",
        "BQ25185_DLH0010A.kicad_mod",
        "BMI270_Bosch_LGA14.kicad_mod",
        "TPS923610_DRL0006A.kicad_mod",
        "FH34SRJ-24S-0.5SH.kicad_mod",
        "FH34SRJ-6S-0.5SH.kicad_mod",
    )
    for name in local_footprints:
        if not (HERE / "ENKU.pretty" / name).is_file():
            errors.append(f"missing local verified footprint source: {name}")

    if "TPS63802DLAR" not in text:
        errors.append("PCB missing TPS63802DLAR first-spin regulator")
    if "TPS63031" in text:
        errors.append("obsolete TPS63031 remains on PCB")
    if "DFE201612E-R47M=P2" not in text:
        errors.append("PCB missing 0.47uH DFE201612E power inductor")
    for required_net in ("SYS_EN", "REG_L1", "REG_L2", "REG_FB", "REG_PG"):
        if f'"{required_net}"' not in text:
            errors.append(f"PCB missing TPS63802 net {required_net}")

    if errors:
        for e in errors:
            print("ERROR:", e)
        print(f"\nENKU PCB placement gate: FAIL ({len(errors)} issue(s))")
        return 1

    print("ENKU PCB placement gate: PASS")
    print(f"  board: {EXPECTED_SIZE[0]:.1f} x {EXPECTED_SIZE[1]:.1f} mm")
    print(f"  placed references parsed: {len(refs)}")
    print("  critical placement refs: present")
    print("  Qi keepout origins: clear")
    print("  copper routing: not started")
    print("\nNOTE: this is a placement/integrity gate, not KiCad DRC.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
