# Roadmap

This roadmap contains future work only. Current verified state is in
[Project status](STATUS.md); completed work remains in the
[experiment index](experiments/README.md).

## Reliability guardrails for performance successors

- Keep Metal argument buffers disabled. Experiment 0049's candidate is rejected
  after three consecutive initial mouse-focus failures, while the subsequent
  OFF control captured focus normally and completed approximately 54 minutes of
  ordinary play.
- Preserve the current inactive 100-ms pacing bypass and bounded compositor
  repair while testing performance changes. Do not force ESO's active byte,
  synthesize AppKit focus events, or call private activation APIs.
- Treat argument buffers as a supported startup-timing hypothesis, not a proven
  direct focus mechanism. Revisit a short inactive yield only if the OFF
  production behavior naturally repeats the mouse-capture failure.
- Preserve the fixed 1920 x 1200 balanced profile as the standard settings
  control. Performance and quality experiments must use separate opt-in
  profiles and must not silently mutate this checkpoint.
- On any natural pink, low-FPS, focus, or reset recurrence, preserve the exact
  run and cache identities before retrying. Do not require launcher restarts or
  delete caches as a workaround.

## P1: isolate an optional pink repair

- Preserve Experiment 0043's decisive two-input result: the GUI-classified
  second image supplies exact magenta at ordinals 80 through 140 and ordinary
  colors at ordinals 150 through 180 while retaining the same image identity.
  The scene-classified first image is not magenta in either interval.
- If a root repair is later preferred over presentation neutralization, trace
  the producer/readiness transition of the proven GUI-classified image. Do not
  add per-frame image readback merely to replace the already bounded repair.
- Keep the proven bounded 79-draw presentation repair as the production
  control; a producer-side successor must independently preserve FPS, focus,
  fail-open forwarding, and update recovery.

## P2: monitor the 0.2.0 production baseline

- Preserve public 0.1.3 as the prior rollback release and keep both release
  identities immutable.
- Observe natural launches instead of forcing repetitive starts. If pink or
  low FPS recurs, capture the exact run before retrying and compare pacing,
  suppression, latch, and finished-gate records without deleting caches.
- Keep official MoltenVK 1.4.2, the inactive-pacing bypass, bounded compositor
  repair, generation-aware recovery, and verified Uninstall path together as
  one supported baseline.
- Treat regressions in ordinary play, live graphics resets, startup color,
  update recovery, or uninstall behavior as release blockers for successors.
- Use public Status and privacy-filtered Diagnostics as the first support path;
  retain source-only traces for a separately scoped exact-target experiment.

## P3: research three 60-FPS-constrained performance directions

Treat 60 FPS as the lower acceptance bound on the validated M4 target, not as
a universal guarantee for every Mac. Share one measurement harness across all
three directions, but keep each runtime or settings change in a separate
single-variable experiment.

### Shared evidence gate

- Use one controlled low-variance scene and one repeatable high-load route.
  Hold zone, camera, resolution, settings, cache generation, player-density
  exposure, power source, and test duration as constant as practical.
- Capture average FPS, 1% and 0.1% lows, frame-interval p95/p99, GPU time,
  application and Metal memory, power, and thermal state. Do not accept the
  in-game FPS counter alone as comparative evidence.
- Preserve warm and naturally occurring cold-cache states separately. Do not
  delete, replace, or distribute a pipeline cache to force a result.
- Establish run-to-run noise before setting the minimum meaningful delta. A
  candidate must exceed that noise, preserve rendering correctness and live
  graphics resets, and retain the Experiment 0044 startup invariants.

### Direction A: maximize FPS at fixed visual quality

1. Keep Experiment 0048's post-window wrapper benchmark as a regression guard,
   but defer self-retiring dispatch. Direct versus cached-wrapper measurements
   found only 8-9 ns for an acquire/present pair, 5 ns per indexed draw, and
   3 ns per descriptor update. That is too small to justify a mutable
   trampoline without whole-frame evidence that another target amplifies it.
2. Exclude Metal argument buffers from further production A/B work. Their
   14.899% non-game descriptor-path gain did not justify three consecutive
   initial mouse-focus failures, and the OFF control passed without that user-
   visible regression. Preserve the benchmark as evidence, not as a pending
   default candidate.
3. Test maximum concurrent pipeline compilation only for startup compilation
   latency and stutter. The retained 64 calls are already fast once ESO issues
   them, so do not expect or claim a steady-state FPS gain without direct
   frame-time evidence.
4. Use VSync-off only during the bounded throughput measurement. The current
   100-FPS interval is a cap, not a performance mechanism; a high-refresh
   display is required to validate visible output above 60 Hz.

### Direction B: maximize visual quality while holding 60 FPS

1. Test `SUB_SAMPLING` from the fixed standard value `1` to High (`2`) first in
   a separate opt-in profile. This has the clearest whole-scene visual benefit
   and belongs to the prior 2048 x 1280, 60-FPS M4 checkpoint.
2. Restore `SHADOWS` from `1` to `2` as a separate A/B while retaining the
   already enabled high-resolution shadow option.
3. Restore `PLANAR_WATER_REFLECTION_QUALITY` from `0` to `1`, then to `2` only
   if the first step holds the floor. Keep screen-space reflection unchanged
   so the result remains attributable.
4. Only after those known checkpoint values pass, evaluate antialiasing,
   post-processing, particle density, view distance, and character resolution
   one at a time in visual-benefit versus GPU-cost order.
5. Reject any step that creates a sustained sub-60 interval in the controlled
   gameplay window, worsens frame-time tails beyond measured noise, or breaks
   rendering after a live graphics reset. The highest passing combination
   becomes an optional, hardware-scoped `Quality 60` profile.

### Direction C: minimize energy and resources at fixed quality and 60 FPS

1. Reuse the post-startup wrapper-retirement candidate from Direction A, but
   judge it by CPU time, package power, frame energy, and thermal behavior at a
   60-FPS cap rather than by uncapped FPS.
2. Benchmark `MVK_CONFIG_SHADER_COMPRESSION_ALGORITHM=3` (`LZ4`) only as a
   retained-MSL-memory trade. It cannot make executing shaders faster; accept
   it only if it materially reduces application/Metal memory without worsening
   cache serialization, startup time, frame-time tails, or power.
3. Compare maximum and non-maximum concurrent compilation for transient power,
   thermal pressure, and compilation stutter. Retain non-maximum compilation
   unless the maximum mode produces an evidence-backed net benefit.
4. Treat macOS Low Power Mode and the 60-FPS cap as external measurement
   factors, never settings that the patch silently changes. Any efficiency
   profile must keep visual settings identical to its reference.

If the three directions produce validated differences, expose them as
explicit, hardware-scoped `Performance`, `Quality 60`, and `Efficiency 60`
choices. Keep the runtime bridge common where possible, apply settings only by
explicit player choice, and publish measured tradeoffs instead of naming one
profile universally best. This program follows the measurement-stripped
Experiment 0044 release; it does not block or mutate that fixed functional
baseline.

## P4: maintain ESO update support

- Run the quick update gate after every ESO or launcher content update.
- Allow the packaged auditor to re-attest relocation-only executable updates
  only when embedded MoltenVK, patch sites, reference shape, proc routes, and
  runtime boundaries remain compatible; any mismatch requires manual analysis.
- Require launcher Repair when an executable update leaves the bridge active;
  never use an older original-loader backup as a substitute for observing the
  vendor's current original generation.
- Add a new supported target only after rebuild, non-game probes, disposable
  install/remove testing, and a user-controlled normal-launch smoke test.
- Never turn an unknown build into best-effort compatibility.

## P5: broaden verified compatibility

- Validate the same exact client through a direct ZeniMax installation when a
  suitable user-controlled test is available.
- Add Apple GPU and macOS versions only with explicit hardware/runtime evidence.
- Keep Steam and launcher discovery edition-neutral; authentication remains the
  responsibility of the user's normal launcher.

## P6: improve distribution

- Collect feedback on the unsigned ZIP's Terminal and Gatekeeper experience.
- Consider a signed/notarized app or DMG if Apple Developer membership becomes
  worthwhile.
- Add release automation only if it preserves exact payload hashes, archive
  hygiene, release notes, and rollback verification.
