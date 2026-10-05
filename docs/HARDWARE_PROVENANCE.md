# ENKU hardware provenance and external engineering references

Status: active engineering record — 2026-10-05

ENKU is developed in public with substantial AI-assisted engineering work. AI is used for research synthesis, design review, calculations, documentation, code generation/review and PCB iteration. The project owner makes the product decisions and directs the work; repository history, CI, datasheets and physical validation are used to verify outputs. AI assistance is not evidence that a circuit is independently authored, so provenance is reviewed separately from technical correctness.

## Provenance policy

For ENKU-owned hardware released under CERN-OHL-P-2.0:

1. Manufacturer datasheets, application notes and panel drawings are the preferred electrical sources of truth.
2. External open-hardware projects may be studied for comparison, failure modes and interoperability.
3. If an external project's concrete schematic/layout/component-selection expression materially informs an ENKU block, that relationship must be recorded before the block is claimed as original ENKU hardware.
4. A reciprocal hardware license is not bypassed by removing attribution or renaming nets. Potentially derived source is quarantined until its license obligations are satisfied or the block is independently re-derived.
5. Historical Git commits are not rewritten to hide the design process.

## Silkscreen review

Project: **Silkscreen**, by Ian Chasse / project contributors.
Repository: https://github.com/iandchasse/silkscreen-pcb
Hardware license at review time: **CERN-OHL-S-2.0**.

Silkscreen was reviewed during ENKU's October 2026 hardware work as an external 4.26-inch ESP32-S3 e-reader reference.

### Confirmed overlap requiring action

The ENKU October 2 frontlight pass changed from an earlier two-driver concept to a single TPS923610 boost and documented an external project as a cross-check. The resulting prototype baseline also used a 15 Ω current set, a 10 µH inductor and low-side warm/cool selection. This combination is sufficiently close to the reviewed Silkscreen frontlight implementation that ENKU will not present that pass as clean, independently authored ENKU fabrication source.

Action:
- preserve the history;
- acknowledge Silkscreen explicitly;
- quarantine the affected frontlight baseline from fabrication;
- independently re-derive the production candidate from current Good Display and LED-driver manufacturer sources;
- run a fresh schematic/layout review before Pro population is released.

### Blocks checked with no equivalent Silkscreen topology found

At this review point, ENKU's core power architecture is materially different: ENKU uses the BQ25185 charger/power-management path, TPS2121 source arbitration target, TPS63802 3.3 V regulator and a hard-off product requirement. The reviewed Silkscreen design uses a TP4056-class charger, TPS2116 source mux, TLV75533 regulator and DW01A/FS8205A cell-protection path.

Repository searches also found no ENKU use of Silkscreen's DS3231 RTC implementation or its specific ADC button-ladder implementation.

This is a scoped provenance finding, not a claim that every line of ENKU hardware has completed final third-party review. The licensing audit remains a release gate.

## Manufacturer-source clean-room rule for the replacement frontlight

The replacement frontlight design must start from:
- the exact delivered Good Display panel/frontlight drawing and electrical limits;
- the selected LED driver's current manufacturer datasheet and application guidance;
- ENKU's own hard-off/power-tree requirements;
- ENKU mechanical and repairability constraints.

The new review must record calculations for current limit, inductor peak/RMS current, capacitor effective capacitance, OVP behavior, channel-selection fault states, startup/shutdown timing, minimum brightness and sleep/hard-off leakage.

Silkscreen may remain in the references/acknowledgements section, but its schematic/layout is not to be used as the drafting source for the replacement block.

## Other external projects

CrossPoint, XTE/XTEINK-class devices, Picco and other readers may be evaluated for compatibility, UX, market comparison or engineering lessons. Such references do not imply source reuse. Any future source-level reuse must be recorded here with its exact license and attribution requirements.
