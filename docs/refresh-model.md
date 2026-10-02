# ENKU Refresh Manager & E-Paper Update Policy

This document defines how ENKU converts render plans into safe e-paper updates.

The goal is to minimize unnecessary panel work, keep interaction predictable and make ghosting/power policy measurable rather than scattered across screens.

## 1. Core principle

UI and Reader code never decide how the physical panel is refreshed.

They describe **what changed**.

The Refresh Manager decides **how and when** the panel is updated.

Conceptually:

```text
App State / Reader
→ UI composition
→ RenderPlan
→ Refresh Manager
→ DisplayService
→ panel driver
```

## 2. Render plan

A render plan describes visible changes independently from the panel driver.

It may contain:

- one or more dirty regions;
- whether the whole screen changed;
- content/change classification;
- urgency;
- whether update may be deferred;
- whether updates may be coalesced;
- whether a full refresh is preferred/required;
- reason/event identifier for diagnostics.

The plan does not contain driver commands.

## 3. Dirty region

A dirty region is a logical screen rectangle.

Conceptually:

```text
x
y
width
height
```

Rules:

- regions are clipped to the current viewport;
- zero-area regions are ignored;
- overlapping/touching regions may be merged;
- screen/orientation changes invalidate previously queued region coordinates.

## 4. Refresh classes

Reader v1 defines four product-level refresh classes:

- **Region** — update one or more dirty areas;
- **Full** — refresh the full visible screen;
- **Deferred** — visible change may wait for a short coalescing/throttling window;
- **None** — state changed but nothing visible changed.

These are product intents. The platform driver may map them to the panel capabilities available on the verified hardware.

## 5. Typical policy by interaction

### Page turn

A page turn changes the complete reading viewport.

Policy:

- render the new reading page completely;
- submit one serialized page update;
- do not queue multiple unbounded page turns;
- exact partial/full waveform choice is hardware-measured.

The architecture does **not** assume that a page turn must always use the hardware driver's full-refresh primitive.

## 6. Focus movement

Changing focus in Library/Settings usually affects:

- previous focused control;
- newly focused control.

Policy:

- dirty only those regions when reliable;
- merge nearby regions when doing so is cheaper/safer;
- avoid full-screen redraw solely for focus movement.

## 7. Overlay open/close

Opening an overlay dirties:

- overlay bounds;
- any background region visually replaced by the overlay.

Closing an overlay must restore the underlying composed content from logical UI state/render buffers rather than rely on the panel to remember pre-overlay pixels.

If overlay geometry or panel behavior makes region restoration unreliable, the Refresh Manager may escalate to a broader/full update.

## 8. Status and progress updates

Examples:

- transfer progress;
- battery/status indicator;
- time where a screen explicitly shows time;
- connection state.

Policy:

- throttle rapidly changing values;
- coalesce updates;
- do not redraw hidden status;
- do not refresh for every numeric increment.

Exact minimum intervals remain implementation/measurement details.

## 9. Orientation change

Orientation change always invalidates the current screen layout.

Policy:

- discard queued old-orientation regions;
- rebuild UI for the new viewport;
- require a full-screen render plan;
- refresh as one serialized operation.

## 10. Language / layout-affecting settings change

If a setting can change text geometry/layout broadly:

- rebuild the visible screen;
- use a broad/full render plan where required;
- preserve logical focus/semantic position separately.

Examples:

- UI language;
- typography;
- margin/layout mode.

## 11. Sleep screen

Before system suspend/power-off:

1. compose the final static sleep/power screen;
2. complete the final required display refresh;
3. wait for panel idle/completion;
4. put the e-paper controller to sleep;
5. only then continue into the selected system power state.

The Refresh Manager must expose completion/failure to the power coordinator.

## 12. Refresh queue

The Refresh Manager owns a bounded queue.

It must prevent:

- concurrent panel refreshes;
- unbounded accumulation of page turns;
- stale update regions after a newer full-screen change;
- repeated redundant updates.

A new plan may:

- merge with a pending plan;
- replace an obsolete pending plan;
- escalate a pending region update to full;
- be rejected/coalesced if an equivalent newer state already exists.

## 13. Coalescing rules

Safe baseline rules:

- overlapping region updates can merge;
- adjacent focus-region updates can merge;
- a pending Full update supersedes older Region updates for the same frame/state;
- orientation/full-layout changes invalidate all older queued regions;
- hidden/invisible-state changes produce no panel work.

Coalescing must preserve the newest logical UI state.

## 14. Serialization

Only one physical display operation may execute at a time.

The manager tracks:

- idle;
- preparing;
- refreshing;
- waiting for panel completion;
- failed/recovery.

Input/runtime may continue handling non-display work, but no second driver refresh starts concurrently.

## 15. Frame/state generation

Render plans should carry a monotonic logical generation/token.

This allows the Refresh Manager to detect stale work.

Example:

```text
state generation 104 → focus moved
state generation 105 → screen changed
pending generation 104 region update is now obsolete
```

The exact counter width is implementation-specific.

## 16. Ghosting policy

Ghosting thresholds are **not fixed before real-panel testing**.

The architecture supports counters/telemetry such as:

- region refresh count since last cleanup/full refresh;
- changed-area accumulation;
- page-turn count;
- elapsed active time;
- screen category;
- explicit artifact observations during test firmware.

The final policy may force a broader/full refresh after measured thresholds.

No guessed threshold is part of Reader v1 specification today.

## 17. Full-refresh escalation

A Region/Deferred request may escalate to Full when:

- dirty area becomes too large;
- too many dirty regions accumulate;
- orientation/layout changes;
- panel driver requires it for the requested update type;
- measured ghosting policy requires cleanup;
- framebuffer/reference state is no longer trustworthy;
- recovery after display error requires resynchronization.

Thresholds remain hardware-profile data.

## 18. Display error recovery

If a refresh fails:

- stop issuing overlapping refreshes;
- report a structured Display error;
- keep AppState authoritative;
- preserve the current render plan/newest logical frame if possible;
- attempt driver/panel reinitialization according to platform policy;
- perform a known-safe full redraw after recovery when required.

The UI state must not be rolled back merely because the physical refresh failed.

## 19. Render buffer ownership

The Refresh Manager does not own Reader semantic state.

It may own or reference:

- current target framebuffer;
- previous/reference framebuffer if the selected driver strategy needs one;
- dirty-region metadata;
- pending update queue.

Exact framebuffer count/location (internal RAM vs PSRAM) is deferred until profiling.

## 20. No invisible redraws

Examples that must produce `None` when not visible:

- reading progress persisted in background;
- trusted Wi-Fi state changes while status is hidden;
- log event;
- cover cache regeneration for an off-screen book;
- metadata update for a book not currently shown.

State updates and persistence are independent from display refresh.

## 21. Reading interaction back-pressure

Reader page turns are display-bound operations.

Baseline behavior:

- while a page update is still in flight, additional page-turn input is not allowed to create an unlimited queue;
- one future logical page request may be coalesced only if implementation proves it predictable;
- default v1 behavior is conservative serialization.

This is consistent with the physical-input policy.

## 22. Diagnostics

Development diagnostics should expose:

- total refresh count;
- region refresh count;
- full refresh count;
- escalations from Region to Full;
- merged/coalesced plan count;
- dropped obsolete plan count;
- display error count;
- refresh duration;
- dirty area/region count;
- current panel/profile capabilities.

These metrics support real-panel tuning.

## 23. Hardware capability profile

The platform display implementation reports capabilities to the Refresh Manager.

Conceptually:

```text
supports_region_refresh
supports_fast_refresh
requires_alignment
min_region_width/height
preferred_region_alignment
panel_busy_available
```

Exact fields depend on the verified driver.

Product/UI code does not branch on raw Waveshare driver APIs.

## 24. Update policy is not a user setting by default

Reader v1 should not expose technical controls such as:

- LUT selection;
- waveform mode names;
- arbitrary "partial refresh count";
- raw ghosting threshold.

A user-facing refresh option should exist only if hardware testing reveals a clear understandable product benefit.

## 25. Decisions fixed by this document

- UI/Reader code produces render plans, not panel commands.
- Refresh Manager owns full/region/deferred/no-update policy.
- Physical display access is serialized.
- Refresh work is bounded and stale plans may be discarded.
- Full-screen/layout changes supersede older region work.
- Orientation changes invalidate queued old-coordinate regions.
- Hidden state changes never refresh the panel.
- Ghosting thresholds remain hardware-measured, not guessed.
- Display failures preserve logical AppState and recover by redraw/reinitialization.
- Refresh diagnostics/counters are required for hardware characterization.
- Reader v1 does not expose low-level waveform/ghosting controls as normal user settings.
