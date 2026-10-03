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
    "L_3V3", "SW_POWER",
    "SW_PREV", "SW_NEXT", "SW_SELECT", "SW_BACK",
}

BOARD = (20.0, 20.0, 74.0, 114.0)
EXPECTED_SIZE = (54.0, 94.0)

# Placement-only coordinate gates. These are broad regions, not final courtyard DRC.
REGIONS = {
    "U1": (20.0, 35.0, 46.0, 55.0),
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
            errors.append("U1 must stay rotated 90 deg with antenna end toward left edge")
    for ref in ("SW_PREV","SW_NEXT","SW_SELECT","SW_BACK"):
        if ref in refs and refs[ref][0] < 68.5:
            errors.append(f"{ref} left the right-side thumb rail")

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

    if "PLACEMENT BASELINE B" not in text:
        errors.append("placement baseline B banner missing")

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
