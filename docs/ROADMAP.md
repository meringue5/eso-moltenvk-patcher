# Roadmap

This roadmap contains future work only. Current verified state is in
[Project status](STATUS.md); completed work remains in the
[experiment index](experiments/README.md).

## P0: isolate the 0.1.2 low-FPS release incident

- Treat Experiment 0040 as falsifying the no-neutralizer profile as a
  reliability repair. The exact 0.1.2 control reproduced pink and low FPS,
  delayed graphics-pipeline call 5 to 32.698 seconds, and delayed renderer
  completion to 13.762 seconds even though all retained calls succeeded
  quickly.
- Exclude compositor neutralization as a necessary cause and do not describe
  0.1.2 as a demonstrated low-FPS fix. Its packaging, exact-target, and
  reversibility claims remain independently verified.
- Treat the exact ESO inactive-loop path as the leading mechanism: when its
  internal application-active byte is false, `GameClient::mainLoop` calls
  `usleep(100000)`, directly imposing an approximately 10-Hz outer loop.
- Keep the exact Experiment 0044 candidate installed for ordinary-use soak
  without forced repeats. Its first two starts had no pink and normal FPS; the
  second recorded `active=no` with `action=sleep-bypassed`, directly exercising
  the patched false-state branch.
- Classify the first natural recurrence against the bounded pacing,
  neutralizer, pipeline, and ESO renderer records before changing the
  candidate. Use a forward-only device/swapchain/queue/present trace only if
  low FPS returns despite a recorded inactive-sleep bypass.
- If ordinary starts remain clean, preserve Experiment 0044's exact functional
  behavior in the measurement-stripped release profile described in P2, then
  publish a new immutable release rather than replacing the 0.1.2 asset.

- Treat Experiment 0035's back-to-back pair as the current discriminator: both
  starts activated the full bridge, but only the smooth restart engaged
  `MTLCompilerService` and completed Metal compilation jobs.
- Treat Experiment 0036 as a failed fix: its process-unique canary reached the
  compiler service and completed both immediate jobs in a later low-FPS
  process, so readiness alone does not trigger ESO's normal pipeline path.
- Treat Experiment 0037 as a failed repair and successful diagnostic: low FPS
  recurred with 64/64 successful fast pipeline calls because ESO delayed
  issuing the bulk wave by about 20.8 seconds.
- Preserve Experiment 0038's first normal result: with neutralization and all
  supporting audits disabled, pink remained visible but FPS and renderer timing
  returned to the normal path.
- Continue classifying naturally occurring starts without forcing repeated
  gameplay; two successful 0044 starts do not yet close the release incident.
- Repair the missing readiness-success production log record before reusing
  that canary as a diagnostic invariant; do not retain it as a claimed fix.
- Classify each future start from direct graphics-pipeline call/return timing
  and user-visible state. Treat compiler-service engagement only as supporting
  evidence: one normal process remained canary-only for more than nine minutes.
- Record ESO and launcher process identity for correlation, but do not make
  launcher restart a required workaround or causal assumption. One four-run
  sequence recovered after a launcher restart, while prior recovery did not
  always require it.
- Snapshot pipeline-cache identity, size, and modification time around every
  start, preserve each naturally occurring generation before the next retry,
  and separate ESO restart effects from launcher restart effects. Two
  consecutive low-FPS exits already rewrote the same-size active cache to
  different hashes without normal ESO compilation.
- Capture fixed-scene FPS, GPU time, frame interval, app/Metal memory, and
  thermal state for the first bad and first clean starts.
- Treat the alternate ESO renderer-initialization path as confirmed: low starts
  pass through the game-data/character-data waits and delay `RENDERER Complete`
  to about 13 seconds versus about 2.7 seconds in the preceding normal start.
  Maximum concurrent compilation has been falsified as a repair; retain cache
  revalidation and other startup resource state as alternatives and do not
  delete or replace caches to force a result.
- Add a prelaunch preparation step only if it can be proven safe without
  starting ESO, bypassing authentication, or distributing proprietary cache
  data.

## P1: isolate an optional pink repair

- Keep visible pink as an accepted cosmetic limitation in 0.1.2.
- Preserve Experiment 0043's decisive two-input result: the GUI-classified
  second image supplies exact magenta at ordinals 80 through 140 and ordinary
  colors at ordinals 150 through 180 while retaining the same image identity.
  The scene-classified first image is not magenta in either interval.
- Experiment 0042 has completed that forward-only source preparation while
  retaining the inactive pacing bypass and non-maximized compilation. Its
  cache-preserving installation passed the shared bundle-idle and identity
  gates, but its first normal-FPS/pink run was inconclusive because the default
  production log filtered every bounded audit record. Experiment 0043's
  dedicated log-policy probe, exact-target cache-preserving installation, and
  ordinary user launch all passed; the result is now owned by Findings and the
  experiment record.
- Experiment 0044 passed twice with no pink and normal FPS. Preserve that
  result in the experiment record and Findings; only the optional underlying
  GUI-placeholder producer repair remains future work here.
- If a root repair is later preferred over presentation neutralization, trace
  the producer/readiness transition of the proven GUI-classified image. Do not
  add per-frame image readback merely to replace the already bounded repair.

## P2: derive and release a measurement-stripped Experiment 0044 repair

- Freeze Experiment 0044's functional behavior, not its diagnostic
  instrumentation: retain the exact inactive 100-ms sleep bypass, MoltenVK
  configuration, fail-open compositor identity checks, 79-draw fixed-window
  suppression, and ordinal-150 forwarding latch.
- Complete the Experiment 0045 non-game gate for the separately prepared
  release profile. Prove that its finished gate leaves every retained wrapper
  in direct-forward operation without lifecycle-table locking, mutation, or
  diagnostic logging.
- Prove functional equivalence with non-game and exact-log probes: the release profile must
  suppress the same exact 79 draws, forward at ordinal 150, fail open on every
  incomplete identity, emit no pipeline-timing records, and show no post-finish
  lifecycle bookkeeping. Keep bounded operational begin/suppress/latch/error
  evidence needed to diagnose a failed repair.
- Treat Experiment 0044's two successful starts as evidence for the functional
  combination only. Require a user-controlled ordinary start of the stripped
  profile before packaging; do not infer binary equivalence merely from disabled
  flags.
- Do not mix the optional performance candidates below into this release or
  invalidate its attribution. A more elaborate self-retiring dispatch/trampoline
  remains follow-up work unless the post-finish direct-forward probe exposes a
  material residual cost.
- After the ordinary-use reliability gate remains clean, promote the exact
  validated release-profile bridge and runtime through a new immutable release
  rather than replacing the existing 0.1.2 asset.
- Keep official MoltenVK 1.4.2 and the verified inactive-sleep and bounded
  compositor repair identities together as one release baseline.
- Preserve the pristine loader, release restore record, historical runtime
  backups, all pipeline-cache generations, and the 48-key sanitized settings
  template with validated High subsampling.
- Keep release install and uninstall transactional, idempotent, and fail-closed
  on unknown files or incomplete recovery state.
- Package the Experiment 0046 generation-aware installer state machine with the
  measurement-stripped runtime candidate. Verify the assembled ZIP repeats the
  retained-bridge repair refusal, external-original preservation, and
  supported-generation rotation fixture before publication.
- Treat regressions in ordinary play, live graphics resets, startup color, or
  restore behavior as release blockers.

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

1. Measure and retire the post-startup bridge wrapper tax. The presentation
   repair latches to forwarding at generation-2 ordinal 150 and its bounded
   audit closes at ordinal 180, but ESO retains the intercepted draw,
   descriptor, submit, and presentation function pointers for the process
   lifetime. Compare direct MoltenVK against the current
   post-window atomic fast paths in a non-game draw/descriptor benchmark. If
   the difference is material, add a self-retiring direct-forward dispatch
   after the verified finish without changing behavior inside the repair
   window. This is the highest-confidence bridge-specific candidate because it
   removes known work rather than changing rendering semantics.
2. Re-evaluate Metal argument buffers only as a high-risk MoltenVK 1.4.2
   experiment. Upstream describes them as a common performance path and 1.4.2
   includes relevant alignment fixes, but ESO's prior single-variable result
   tied them to rendering corruption. Require argument-buffer-on descriptor,
   reset, shader-output, and exact ESO-era resource-shape probes before one
   user-controlled game A/B. Never make this the default on theoretical gain.
3. Test maximum concurrent pipeline compilation only for startup compilation
   latency and stutter. The retained 64 calls are already fast once ESO issues
   them, so do not expect or claim a steady-state FPS gain without direct
   frame-time evidence.
4. Use VSync-off only during the bounded throughput measurement. The current
   100-FPS interval is a cap, not a performance mechanism; a high-refresh
   display is required to validate visible output above 60 Hz.

### Direction B: maximize visual quality while holding 60 FPS

1. Restore `SUB_SAMPLING` from the current reduced value to High (`2`) first.
   This has the clearest whole-scene visual benefit and already belongs to the
   prior 2048 x 1280, 60-FPS M4 checkpoint.
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
