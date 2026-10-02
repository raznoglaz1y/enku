# ENKU Input & Physical Controls

This document defines the ENKU logical input model using the controls that Waveshare exposes on the ESP32-S3-ePaper-3.97.

It separates **vendor-confirmed hardware behavior** from **ENKU product mappings**.

## 1. Vendor-confirmed controls

Waveshare documentation describes:

- one onboard three-direction control ("rotary button");
- a side **BOOT** button;
- a side **PWR** button.

The current official ESP-IDF example does not treat the three-direction control as a quadrature rotary encoder. It exposes three independent active-low GPIO inputs:

| Vendor name | GPIO | Active level |
| --- | ---: | ---: |
| Button Up | GPIO4 | LOW |
| Button Function | GPIO5 | LOW |
| Button Down | GPIO6 | LOW |
| BOOT | GPIO0 | LOW |

The official example enables pull-ups and polls these inputs through the bundled multi-button state machine.

### Vendor polling model

The current Waveshare example:

- starts a periodic timer every **5 ms**;
- calls the button state-machine tick function;
- recognizes click, double-click, press-down, release, repeat, long-press start and long-press hold;
- publishes resulting button events through a FreeRTOS Event Group.

This means ENKU can build its logical input layer without depending on GPIO interrupts.

## 2. Vendor example behavior

The current Waveshare UI examples use the controls approximately as follows:

- **Down single click** → next item / next page;
- **Up single click** → previous item / previous page;
- **Function single click** → enter / confirm / open an action;
- **Function long press** → manual full e-paper refresh in several screens;
- **BOOT double click** → back / exit in file-browser and reader flows;
- **BOOT long press** → Settings from the vendor home page;
- **PWR** is handled through the AXP2101 power-management path rather than as a normal navigation GPIO.

The vendor reader example also supports double-click shortcuts, but ENKU does not adopt those as a product requirement.

## 3. PWR behavior

The official AXP2101 setup configures:

- power-on press time: **1 second**;
- hardware power-off press time: **4 seconds**.

ENKU treats PWR as a system/power control, not a normal navigation key.

The exact behavior of short PWR presses, wake sources and hard-power-off interaction will be verified on the real board before implementation is frozen.

## 4. ENKU physical input names

The platform layer should expose physical controls using stable names rather than raw GPIO numbers:

```text
PhysicalControl::Up
PhysicalControl::Function
PhysicalControl::Down
PhysicalControl::Boot
PhysicalControl::Power
```

GPIO mapping stays inside the board/platform implementation.

## 5. ENKU press types

The input layer may recognize:

```text
Press
Release
Click
LongPress
Repeat
```

**Double-click is not part of the ENKU v1 interaction model.**

Reason:

- it delays confirmation of a normal click;
- it is difficult to discover;
- it is unnecessary with the available physical controls;
- e-paper interaction benefits from predictable one-action-per-press behavior.

The low-level input driver may still detect it for diagnostics if useful, but application screens must not require it.

## 6. Logical actions

Physical controls are translated into logical actions before reaching screen/runtime code.

Initial actions:

```text
NavigatePrevious
NavigateNext
Confirm
Back
OpenReaderMenu
OpenQuickTypography
PagePrevious
PageNext
Sleep
Wake
PowerOff
```

Screens consume logical actions, not GPIO numbers.

## 7. Global navigation mapping

For list/menu/settings contexts:

| Physical input | ENKU action |
| --- | --- |
| Up click | NavigatePrevious |
| Down click | NavigateNext |
| Function click | Confirm |
| BOOT click | Back |

This is the canonical baseline.

Focus navigation may reinterpret Previous/Next spatially according to a component's deterministic traversal rules, but the physical mapping remains stable.

## 8. Reading mapping

In normal Reading:

| Physical input | ENKU action |
| --- | --- |
| Up click | PagePrevious |
| Down click | PageNext |
| Function click | OpenReaderMenu |
| Function long press | OpenQuickTypography |
| BOOT click | Back to Library |

Returning to Library saves/checkpoints progress according to the runtime persistence policy.

## 9. Reader overlays and dialogs

Once an overlay/menu is open, the controls return to standard navigation semantics:

| Physical input | Action |
| --- | --- |
| Up click | Previous focus item |
| Down click | Next focus item |
| Function click | Confirm |
| BOOT click | Close / Back |

This keeps the interaction model consistent.

## 10. Library mapping

Library Grid/List uses:

- Up → previous item in traversal order;
- Down → next item in traversal order;
- Function → open selected book/details/action;
- BOOT → previous screen/context.

For multi-column Grid, navigation remains deterministic. Exact spatial traversal is owned by the UI/navigation specification rather than by the GPIO layer.

## 11. Keyboard mapping

The on-device keyboard uses the same four navigation controls.

Initial rule:

- Up / Down move through the keyboard's deterministic focus order;
- Function selects the focused key;
- BOOT performs Back/cancel at the keyboard-context level.

If the real hardware ergonomics make a two-axis keyboard impractical with only Previous/Next, the keyboard traversal algorithm may use row grouping, but no additional physical controls are assumed.

## 12. Long press and repeat

### Up / Down

Long-hold repeat is useful for:

- long lists;
- Library traversal;
- settings values;
- keyboard navigation.

It should be rate-limited and accelerated conservatively.

### Reading page turns

Held Up/Down **must not enqueue an uncontrolled stream of page turns**.

Reader v1 should treat a held page-turn control as one page action unless a later measured implementation introduces safe paced repeat.

The next page action is accepted only after layout/display state is ready.

### Function

Function long press opens **Quick Typography (Aa)** while reading.

In menus/settings, Function long press has no required v1 action.

### BOOT

BOOT long press is reserved. ENKU does not require it for ordinary navigation.

This avoids conflicts with boot/download behavior and leaves room for future recovery/service functions.

## 13. Sleep and wake

Sleep is primarily entered by:

- Quick Menu → Sleep;
- configured auto-sleep.

Wake behavior depends on which physical inputs can reliably wake the target hardware and will be verified on the board.

The application-level model therefore exposes `Wake` independently from a specific key until hardware testing is complete.

## 14. Power control

Power and navigation remain intentionally separate.

ENKU v1 policy:

- navigation buttons must not hard-power-off the device;
- PWR remains the dedicated physical power control;
- software Power Off may also be exposed from Settings/Quick Menu;
- hardware long-press shutdown behavior must not be replaced by an application gesture.

## 15. Debounce and event generation

The vendor example proves that a 5 ms polling/state-machine approach works with this board.

ENKU can start from the same architectural pattern:

```text
GPIO sample
→ debounce/state machine
→ PhysicalInputEvent
→ logical action mapping
→ AppEvent
```

Exact timing constants remain implementation details to tune on the real unit.

## 16. Orientation independence

Physical meaning must not invert when the screen rotates.

Up remains Previous and Down remains Next at the logical-product level.

Reader page direction also remains:

- Up → previous page;
- Down → next page.

This avoids orientation-dependent muscle-memory changes.

## 17. Recovery constraints

Because BOOT is GPIO0 and has a bootloader role, ENKU must not rely on holding BOOT during power-on for a normal product feature.

Recovery/service mappings involving BOOT must be designed so they do not interfere with firmware download mode.

## 18. Decisions fixed by this document

- The three-way onboard control is modeled as Up / Function / Down.
- ENKU uses GPIO4 / GPIO5 / GPIO6 and BOOT GPIO0 only inside the platform layer.
- Inputs are active-low according to the current official vendor example.
- PWR is separate from normal navigation.
- ENKU v1 does not require double-click gestures.
- Up/Down/Function/BOOT clicks map to Previous/Next/Confirm/Back in normal UI.
- Reading maps Up/Down to Previous/Next page.
- Function click opens Reader Menu; Function long press opens Quick Typography.
- BOOT click returns/back; BOOT long press is reserved.
- Held navigation may repeat in lists, but page-turn repeat is suppressed unless explicitly paced.
- Wake-source details remain pending real-board verification.


## 19. Concrete input runtime MVP

The framework-neutral input mapper is now implemented as `InputActionMapper`.

It consumes:

```text
AppState + PhysicalInputEvent
→ optional LogicalAction
```

Current behavior matches the product rules above:

- Library/menu Up click or repeat → NavigatePrevious;
- Library/menu Down click or repeat → NavigateNext;
- Function click → Confirm;
- BOOT click → Back;
- Reading Up click → PagePrevious;
- Reading Down click → PageNext;
- Reading Function click → OpenReaderMenu;
- Reading Function long press → OpenQuickTypography;
- Reading Up/Down repeat is discarded;
- any ordinary click while Sleep is active → Wake;
- Power long press maps to PowerOff at the logical layer.

Press and Release are retained for diagnostics/state tracking but do not produce application actions by themselves.

## 20. ESP-IDF button driver

`EspIdfButtons` is the first board-specific implementation for GPIO4/GPIO5/GPIO6/GPIO0.

Current bring-up constants:

```text
poll interval      5 ms
debounce           20 ms
long press         650 ms
repeat delay       700 ms
repeat interval    180 ms
```

All four inputs use internal pull-ups and are active-low.

The driver emits at most one event per poll and rotates its starting scan position after an emitted event so simultaneous inputs do not permanently privilege one GPIO.

The firmware smoke test opens a 12-second diagnostic window after storage/display bring-up and logs:

```text
control
press type
mapped logical action
```

This allows real-board tuning without changing Library/Reader code.


## 21. Runtime dispatch bridge

`InputDispatcher` now connects logical actions to the existing application controllers.

Current routed actions:

```text
NavigatePrevious → LibraryFocusPreviousRequested
NavigateNext     → LibraryFocusNextRequested
Confirm          → OpenFocusedBookRequested in Library
PagePrevious     → Reader PagePreviousRequested
PageNext         → Reader PageNextRequested
Back             → Reader BackRequested while Reading
Wake             → SleepWakeCoordinator::wake()
PowerOff         → PowerOffCoordinator::powerOff()
```

`OpenReaderMenu`, `OpenQuickTypography` and software `Sleep` remain explicit unhandled actions until their overlay/quick-menu runtime layers are implemented.

This keeps physical input translation independent from screen/controller implementations while providing one central dispatch point for the first device runtime graph.
