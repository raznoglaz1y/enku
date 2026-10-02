# ENKU Power, Sleep & Wake Model

This document defines the ENKU product-level power model and separates **display sleep**, **device suspend**, and **full power-off**.

It is grounded in the current official Waveshare ESP32-S3-ePaper-3.97 source, while keeping hardware-dependent wake behavior provisional until measured on the real board.

## 1. What the current Waveshare example actually does

The current official Waveshare ESP-IDF application uses two distinct mechanisms:

### E-paper sleep

The panel driver implements `EPD_Sleep()` by sending the panel's deep-sleep command and then holding the e-paper reset line low.

This sleeps the **display controller**, not automatically the whole ESP32-S3 system.

In the vendor application, after `EPD_Sleep()` the firmware continues running and polling/waiting for button events.

### PMU power-off

The board uses an AXP2101 power-management IC.

The official `axp_pwr_off()` path ultimately calls:

```text
axp2101.shutdown()
```

The current vendor PMU setup also configures:

- PWR hold to turn on: approximately 1 second;
- PWR hold to force power-off: approximately 4 seconds.

### No confirmed ESP32 deep-sleep product flow

The current full Waveshare application does **not** use an ESP32 deep-sleep flow as its normal idle/sleep behavior.

The vendor application instead:

1. puts the e-paper panel to sleep;
2. keeps the firmware alive for a period;
3. eventually shuts the board down through the AXP2101.

Therefore ENKU must not assume that ESP32 deep sleep, arbitrary-button wake, or wake-current behavior is already proven on this board.

## 2. ENKU power states

ENKU distinguishes four logical power states:

```text
Active
DisplayIdle
Suspended
PoweredOff
```

### Active

Normal operation:

- MCU active as required;
- input active;
- display available;
- storage/network enabled according to current task.

### DisplayIdle

Temporary low-activity state where:

- the e-paper controller may be asleep;
- the displayed image remains visible;
- application/runtime may still be alive;
- this state is **not** presented to the user as true device Sleep.

This is mainly an implementation optimization and recovery state.

### Suspended

This is the user-facing ENKU **Sleep** state.

Goals:

- static sleep screen remains on e-paper;
- reading/application state is persisted;
- Wi-Fi is off;
- unnecessary peripherals are off or quiescent;
- power use is substantially lower than Active;
- wake restores the previous logical context.

The exact hardware mechanism for Suspended is **not frozen yet**.

Candidates to validate:

1. ESP32-S3 deep sleep with a verified wake source;
2. PMU-assisted low-power mode if the board supports a useful configuration;
3. controlled PMU power-off with fast logical restore, if true suspend is not practical.

The product behavior is fixed; the electrical implementation remains measurement-driven.

### PoweredOff

Full logical power-off:

- dirty state is persisted;
- a final static screen may be rendered;
- e-paper is put into sleep;
- storage/network operations are stopped safely;
- the AXP2101 shutdown path is invoked.

Wake from PoweredOff is expected to behave as a cold boot followed by logical state restoration.

## 3. Why DisplayIdle is not ENKU Sleep

The official Waveshare example proves that putting only the panel to sleep does not mean the whole device is sleeping.

For an e-reader, calling that state "Sleep" would be misleading and could waste battery.

ENKU therefore reserves the product term **Sleep** for the Suspended state, where system-level current has been measured and accepted.

## 4. Canonical Sleep flow

User-facing Sleep is entered from:

- Quick Menu → Sleep;
- configured auto-sleep.

Canonical transition:

```text
Active
→ SleepRequested
→ block/finish unsafe writes
→ persist dirty state
→ stop Wi-Fi
→ quiesce storage/peripherals
→ render static sleep screen
→ final e-paper refresh
→ EPD sleep
→ enter verified system suspend mechanism
→ Suspended
```

If system suspend cannot be entered safely, ENKU should fail safe rather than silently pretend the device is asleep.

## 5. Canonical Wake flow

Conceptual wake behavior:

```text
Suspended
→ wake source
→ initialize required platform services
→ validate persistent state/storage
→ restore previous logical context
→ repaginate current book if needed
→ render
→ Active
```

The framebuffer is never authoritative across sleep.

The semantic reading position and persisted application state are authoritative.

## 6. Wake sources

Wake sources are hardware-dependent and remain uncommitted until tested.

Candidates to verify on the real board include:

- PWR;
- one or more navigation controls if they can be used as low-power wake inputs;
- RTC/PMU-related wake paths exposed by the board.

The current Waveshare source exposes an RTC interrupt input and PMU-related power logic, but ENKU will not claim any specific wake route until it is physically validated.

## 7. Auto-sleep

Auto-sleep is user-configurable.

It must be suspended while ENKU is performing unsafe-to-interrupt operations such as:

- active book import;
- transactional replace;
- metadata/database commit;
- cover write;
- firmware update in a future revision.

Normal reading inactivity may enter Sleep.

The exact default timeout will be selected after usability and current measurements.

## 8. Power Off

Power Off is distinct from Sleep.

User-visible Power Off may be initiated from:

- Settings;
- Quick Menu;
- physical PWR behavior where appropriate.

Canonical flow:

```text
PowerOffRequested
→ stop accepting new destructive operations
→ persist dirty state
→ flush/close storage
→ stop network
→ render optional power-off screen
→ EPD sleep
→ PMU shutdown
```

The software path should use the verified PMU shutdown mechanism rather than merely entering an infinite loop.

## 9. Emergency / hardware shutdown

The AXP2101's hardware long-hold shutdown remains independent of ENKU's graceful software flow.

ENKU cannot guarantee persistence if the user forces an electrical shutdown while a write is in progress.

Therefore:

- important reader progress is checkpointed before long idle periods;
- transactional storage operations are designed to survive interruption;
- the UI should avoid unnecessary long writes.

## 10. Reading progress and power transitions

Before Sleep or Power Off, ENKU should checkpoint:

- current book;
- semantic reading position;
- reading state;
- per-book settings if dirty;
- Library state if dirty.

The page image itself does not need to be persisted.

## 11. Wi-Fi policy

Wi-Fi is not required while reading or sleeping.

Before Suspended:

- local web sessions are closed;
- active imports must finish/fail safely;
- Wi-Fi is stopped.

On wake:

- Wi-Fi should not automatically power up solely because the device woke;
- normal saved/trusted-network policy decides whether connectivity is needed.

This prevents connectivity from becoming a hidden battery cost.

## 12. E-paper policy

E-paper retains a static image without continual redraw.

Before entering Suspended or PoweredOff:

1. compose the final sleep/power image;
2. perform the required refresh;
3. wait for panel operation completion;
4. call the panel sleep routine.

No live clock or animation exists in Sleep.

## 13. Low battery behavior

Exact voltage and percentage thresholds remain measurement-dependent.

Product behavior should distinguish:

- low battery warning;
- critical battery;
- charging.

At critical battery:

- stop nonessential network activity;
- reject or defer nonessential write-heavy operations where practical;
- persist reader state;
- enter a safe low-power/off state before uncontrolled brownout.

Thresholds must not be guessed before battery/PMU calibration.

## 14. USB / charging behavior

The vendor PMU layer can detect VBUS presence.

ENKU may use this later for:

- charging status;
- modified auto-sleep policy while externally powered;
- safe firmware/service mode.

However, USB-connected behavior is not yet a fixed Reader v1 interaction rule.

## 15. PowerService contract

The platform-facing power service should expose product capabilities rather than AXP2101-specific calls.

Conceptually:

```text
PowerService
  batteryState()
  externalPowerPresent()
  canSuspend()
  requestSuspend()
  requestPowerOff()
  wakeReason()
```

The higher-level runtime must not call `axp2101.shutdown()` directly.

## 16. Measurement plan

On hardware arrival, measure at least:

- Active / Library idle current;
- Active / Reading idle current;
- Wi-Fi connected current;
- panel refresh peak/current profile;
- panel asleep while MCU remains active;
- candidate ESP32-S3 deep-sleep current;
- PMU-powered-off current;
- charging current/behavior;
- wake latency;
- state restoration latency.

Only after these measurements should the final Suspended implementation be selected.

## 17. Decisions fixed by this document

- Panel sleep and device sleep are different states.
- `EPD_Sleep()` alone is not treated as ENKU user-facing Sleep.
- User-facing Sleep means system-level Suspended behavior with measured low power.
- PMU power-off is a separate full shutdown state.
- ENKU will not claim an ESP32 deep-sleep/wake configuration before hardware verification.
- Sleep/Power Off checkpoint semantic reading state before transition.
- Wi-Fi is shut down for Sleep.
- No live clock/animation is maintained during Sleep.
- AXP2101-specific implementation stays inside the platform layer.
