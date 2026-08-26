# Experiment 0040: no-neutralizer low-FPS recurrence

- Date: 2026-08-27
- Outcome: **failed as a reliability repair; diagnostic recurrence captured**
- Rollback: **not performed; verified 0.1.2 control remains installed**

## Question

Does the exact 0.1.2 `startup-pipeline-timing-control` profile continue to
avoid the intermittent low-FPS startup path after compositor neutralization
and all of its supporting audits have been removed?

## Trigger and safety state

The user reported a naturally occurring launch with both the original pink
startup surface and low FPS. Before any retry or cache intervention, the
installed state was checked non-destructively:

- ESO remained exact target 12.0.8/databuild `3288357`, executable SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`;
- the current bridge, enable marker, official MoltenVK 1.4.2 runtime, restore
  path, and all three pipeline-cache identity headers passed;
- exact bridge run `20260826T173857.097445000Z-pid322` selected
  `startup-pipeline-timing-control`, redirected all 17 Vulkan entry points,
  set `maximize_concurrent_compilation=0`, and recorded
  `compositor_neutralize=disabled`; and
- no game, launcher, installed file, cache, or setting was changed by the
  investigation.

The complete local logs and post-exit cache generations were preserved in the
ignored evidence directory
`artifacts/experiment-0040-no-neutralizer-low-recurrence-20260827T023857KST/`.

## Result

The recurrence has the same direct timing signature as the earlier
neutralized low-FPS run:

```text
visual result:                        pink present, FPS low
pipeline timing begins/ends:          64 / 64
pipeline results:                     64 VK_SUCCESS, non-null output each
first pipeline call:                  9.971 s
bulk wave beginning at call 5:       32.698 s
call 5 duration:                      1.425 ms
CharacterSelect -> RENDERER Complete: 13.762 s
initial OnDeviceReset:                4.650 ms
replacement OnDeviceReset:            0.285 ms
```

The active pipeline cache was naturally rewritten on exit to SHA-256
`11a0b3fd3fa2fc289129c097ffb812b3664b2ac174d3298ff0f0bc91e565ebf1`.
`ShaderCache.cooked` remained unchanged at
`055a55c821b5dfeb8db4d1f7d290ce8003a46924449d1a190afcde5a19822f6c`
and retained its 2026-08-21 modification time. Cache identity validation still
passed; byte evolution of the active pipeline cache remains an observation,
not proof of cache causality.

## Interpretation

Confirmed: 0.1.2's exact no-neutralizer profile can enter the pink/low-FPS
state. Removing compositor substitution and its tracking does not prevent the
fault. Experiment 0038's first normal launch was an intermittent positive
sample, not a demonstrated repair, and the neutralizer is excluded as a
necessary cause of low FPS.

Confirmed: neither graphics-pipeline creation latency nor a Vulkan pipeline
failure caused the observed startup delay. ESO waited roughly 21 seconds
longer than the first normal 0038 run before issuing the bulk graphics-pipeline
wave, then every retained call completed quickly. The approximately
13.8-second `CharacterSelect` to `RENDERER Complete` interval remains the
strongest direct classifier for the alternate path.

The trigger remains upstream of `vkCreateGraphicsPipelines`. The next
diagnostic boundary is a forward-only timestamp trace of device, swapchain,
queue, and presentation readiness before the delayed bulk wave. Launcher
lifetime and cache evolution remain correlations, not required workarounds or
causal assumptions.

## Static control-flow amendment: 2026-08-27

Read-only analysis of the exact attested ESO executable identified a direct
10-FPS path outside MoltenVK. `GameClient::mainLoop` calls
`platformMainLoop`, checks an internal application-active byte, and calls
`usleep(100000)` before the next iteration whenever that byte is false. The
100,000-microsecond delay is an exact 0.1-second outer-loop sleep.

The reproducible image-relative locations for this exact target are:

```text
GameClient::mainLoop inactive branch: 0x30f78
branch/sleep bytes:                    75 27 bf a0 86 01 00 e8 8a 78 8c 03
active-state setter:                   0x164792
applicationDidBecomeActive:            0x31ed0
applicationDidResignActive:            0x31f22
internal application-active byte:      0x4a0c93c
```

These offsets are diagnostic fingerprints for the attested executable only;
they are not portable patch locations for any other ESO build.

The same binary implements `applicationDidBecomeActive:` and
`applicationDidResignActive:`. They are the only direct control transfers to
the internal active-state setter found by the static scan, passing true and
false respectively. The active byte is read directly by the outer main loop;
the user-visible background-FPS settings are a separate registered graphics
setting path. The preserved configuration selected a 60-FPS background cap,
so that setting does not account for the observed approximately 10-FPS mode.

This changes the leading interpretation. The low-FPS symptom is consistent
with ESO retaining a false internal application-active state while its custom
main loop continues to render at the hard-coded inactive cadence. Earlier
Experiment 0010 WindowServer evidence recorded ESO as frontmost and receiving
keyboard focus during a comparable approximately 8-FPS/cursor-mode start,
which weakens OS-level background throttling and supports an internal stale or
misordered activation state. The exact event-order defect is still a
hypothesis because no failing run recorded the internal byte transition.

Pink startup output remains a separate rendering symptom: this inactive-loop
sleep exists in ESO's AppKit control flow and does not depend on compositor
neutralization or graphics-pipeline compilation.

The next single-variable candidate is therefore an exact-target, reversible
runtime bypass of only the inactive-loop `usleep(100000)` branch, while
retaining the official MoltenVK runtime, current compilation policy, visible
pink output, settings, caches, focus event propagation, and normal launcher
path. A candidate must validate the complete original instruction sequence,
restore RX permissions on every path, log when the stale inactive state is
observed, and remain fail-closed on any unknown executable.
