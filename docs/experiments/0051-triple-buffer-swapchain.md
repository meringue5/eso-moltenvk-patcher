# Experiment 0051: measured triple-buffer swapchain

- Date: 2026-08-28
- Outcome: **running; corrected triple-buffer B installed**
- Rollback: **verified pristine loader available; caches preserved**

## Question

Does requesting three FIFO swapchain images instead of ESO's observed two
reduce drawable starvation and frame-time tails at the same visual settings,
without regressing the production startup, focus, rendering-reset, or recovery
behavior?

## Hypothesis

The current two-image FIFO chain can make the CPU wait for an image whenever
the display/GPU retains both backings. A third image should reduce the long tail
of `vkAcquireNextImageKHR` and therefore the p99/p99.9 present-entry interval in
GPU loads that remain close to the 60-FPS budget.

The hypothesis is falsified if the candidate does not produce three returned
images, remains within control run-to-run noise, merely moves waiting from
acquire to present, increases latency or memory unacceptably, or causes any
visible startup, focus, rendering, reset, or stability regression.

## Target and change set

- Exact ESO 12.0.8 databuild `3288357`, executable SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- Official replacement MoltenVK 1.4.2.
- Production runtime behavior is retained: Metal argument buffers off, inactive
  pacing bypass active, and bounded compositor neutralization active.
- `startup-release-swapchain-control` measures the unchanged two-image request.
- `startup-release-triple-buffer` copies the create info and changes only an
  exact request of two images to three, after a successful capabilities query
  proves `minImageCount <= 3` and `maxImageCount == 0 || maxImageCount >= 3`.
- Missing capability evidence, an unexpected request, or an unsupported maximum
  forwards ESO's original request unchanged.
- Both modes exclude the first 300 presents of each swapchain and collect the
  same bounded acquire-call, present-call, and present-entry-interval samples.
  A checkpoint after 600 samples and then every 3,600 samples reports p50,
  p95, p99, p99.9, errors, promotion counts, and actual returned image counts.
  A final exit summary remains a best-effort exact-sample record. No per-frame
  log is emitted.

## Preflight

- `scripts/check-update.sh` reports `CURRENT` for the exact target.
- `scripts/status.sh` reports the intact production bridge, current MoltenVK,
  verified recovery, and both preserved pipeline-cache generations.
- A fresh `scripts/build.sh` passes all existing probes and the new swapchain
  policy/configuration probes.
- The optimized non-game wrapper benchmark measures approximately 0.1-0.25 us
  per instrumented acquire/present pair across rebuild runs, less than 0.002%
  of one CPU second at 60 pairs/s. Both A and B arms use the instrumented path.
- The full-resolution AppKit/Metal probe passes outside the restricted sandbox:
  a three-image request returns exactly three images and cycles indices 0, 1,
  and 2 through both swapchain generations and 180 presents, with correct final
  pixels after the resolution-generation transition.
- Before each bundle mutation, repeat the exact update, restore, source-build,
  and shared bundle-idle gates. Preserve caches and settings byte-for-byte.

The launcher's last complete `noUpdateRequired` repository snapshot was about
19.5 hours old, so it was stale under the ordinary one-hour quick-check window.
For evidence preparation only, the snapshot was retained under an explicit
24-hour maximum and recorded as `CURRENT_REMOTE`; every repository identifier
matched. This did not replace the current executable/content fingerprint or the
separate bundle-idle gate, both of which passed immediately before mutation.

## Procedure

Use an A1-B-A2 sequence so the candidate must beat ordinary run-to-run noise:

1. Install `startup-release-swapchain-control` with existing caches preserved.
2. The user launches normally through Steam and the ZeniMax launcher. After
   entering the fixed route, keep zone, camera, 1920 x 1200 Balanced settings,
   power mode, and test duration unchanged. Do not change graphics options.
3. Run the repeatable high-load route for 8-10 minutes. At the fixed capture
   point, wait 20 seconds and record Metal HUD FPS, GPU time, app memory, Metal
   memory, and thermal state. Report visible stutter, input feel, focus, pink,
   and rendering correctness.
4. After normal exit, collect the bridge summary and verify an exact two-image
   chain, zero acquire/present errors, and sufficient post-warmup samples.
5. Restore the verified original loader, preserve all cache states, install
   `startup-release-triple-buffer`, and repeat the identical user-controlled
   route and capture. Verify exact three-image creation for every generation.
6. Restore and reinstall the control, then repeat once as A2.
7. Compare B against the A1-A2 range. Average FPS is secondary when all arms
   remain capped near 60; primary endpoints are acquire p99/p99.9, present-call
   p99/p99.9, present-entry interval p99/p99.9, and their 1%/0.1%-low proxies.

The agent does not launch ESO, Steam, or the launcher. A run stops immediately
on focus loss, pink/low-FPS recurrence, corruption, crash, reset failure, or an
unexpected swapchain policy/count record.

## Acceptance

- All relevant candidate swapchains are proven as exactly three images; control
  remains exactly two. No fallback, count mismatch, or acquire/present error is
  accepted as performance evidence.
- The candidate must improve acquire or full-frame tail latency beyond the
  A1-A2 noise band, not just change average FPS. A slower present tail that
  cancels the acquire gain is not an improvement.
- Startup color, focus, perceived latency, live graphics reset, rendering,
  stability, and the fixed 60-FPS lower bound must not regress.
- The extra app/Metal memory and any power or thermal difference are recorded
  as costs. No universal default is proposed from one machine or scene.

## Evidence

Confirmed before installation:

- Policy probes: supported 2-to-3 promotion, max-two forward fallback, and an
  unchanged two-image control all pass.
- Full-resolution non-game rendering proves three returned images and correct
  generation transition behavior.
- Measurement overhead is below 0.25 us in the non-game probe and common to
  both arms.

A1 installation checkpoint:

- Source commit: `d23458698f155e3c05f4c4a359e6b51b2e349f16`.
- The shared idle gate found ESO and the launcher absent, no open file in the
  bundle, and idle Steam with no ESO content operation.
- The production bridge was restored to its verified pristine loader with all
  cache generations preserved, then `startup-release-swapchain-control` was
  installed from the fresh build.
- Post-install update status remains exact and the marker is exactly
  `startup-release-swapchain-control`.
- Active and old-backup pipeline-cache SHA-256 values remained respectively
  `4f3baa1e13bc25c158f7cd3d274ebae138165d3ba9c1ff5380cee29efa076f60`
  and `72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

First A1 run:

- Run `20260828T102522.338344000Z-pid54063` activated the exact control mode,
  all 17 redirects, the bounded compositor repair, and the ordinal-180 finish.
- The user completed play and no crash report was created. Settings remained
  byte-for-byte identical.
- The active pipeline cache advanced from SHA-256 `4f3baa1e...076f60` to
  `dc4412c9...1fe86`; the old-backup cache remained byte-for-byte identical.
- No `SWAPCHAIN_EXPERIMENT_SUMMARY` reached the production log. Confirmed root
  cause: the new experiment prefix fell through the log policy to `debug`, so
  the normal `info` profile discarded it. Destructor execution cannot be
  inferred from the absence of a row that policy had already filtered.
- This run is valid for activation, settings, cache, and crash evidence, but is
  excluded from the quantitative A1-B-A2 comparison.
- The user reported that initial mouse focus did not attach to the game. The
  periodic/final experiment rows were filtered, so log-file I/O cannot explain
  this occurrence. The common acquire/present timing wrapper was active from
  the first frame, however, and a natural recurrence also remains possible.
  This focus result is a regression observation, not proof of either cause.

The source now classifies only bounded checkpoint/final-summary rows as `info`,
emits in-run histogram checkpoints after 600 samples and every 3,600 samples,
and keeps per-create/per-frame details below production visibility. The analyzer
accepts the latest checkpoint when a final summary is absent. The optimized
policy probe and 142 Python tests pass after this amendment.

Before B, the timing wrapper was further gated by the existing bounded startup
window: acquire and present forward directly, without clocks, mutexes, sampling,
or checkpoint logging, through generation 2 ordinal 180. Measurement begins on
the following frame. Swapchain capability/count tracking remains active because
B must still prove the actual three-image creation. The policy probe proves the
startup gate. Focus remains a mandatory B and corrected-control acceptance
check; B is not compared numerically with the invalid first A1.

B installation checkpoint:

- Source commit: `28fdc515`.
- Fresh build, policy probe, startup-gate probe, and 142 Python tests passed.
- The shared idle gate found ESO and the ZeniMax launcher absent and Steam idle
  with no ESO file/update operation.
- A1 was restored through the verified pristine loader, then
  `startup-release-triple-buffer` was installed with settings and every cache
  generation preserved.
- The installed bridge and fresh build match at SHA-256
  `e429d14e3a1cc9d8e9c41e12922ecf02fe49e78e022a450d4250c503f4bee737`;
  MoltenVK matches the exact target runtime.
- Active and old-backup cache SHA-256 values remained respectively
  `dc4412c9f97a30cdc9c925e8fd37ee42f782c33393bce0c5693ba7ebc571fe86`
  and `72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

B gameplay checkpoint:

- Run `20260828T105055.379587000Z-pid58260` activated the exact triple mode,
  all 17 redirects, the production compositor/pacing behavior, ordinal-150
  forwarding, and the ordinal-180 measurement gate.
- Both observed swapchain generations requested two images, were promoted to
  three, and returned exactly three. There were zero forwarded/fallback creates,
  capability misses, count mismatches, acquire errors, or present errors.
- The final exact summary reached the 65,536-sample bound for acquire, present,
  and present-entry interval. Acquire p95/p99/p99.9 were 9/12/19 us; present
  call p95/p99/p99.9 were 12/16/24 us.
- Present-entry interval p50/p95/p99/p99.9 were 16.699/18.610/20.230/36.880 ms.
  Inverting p99 and p99.9 gives 49.43 and 27.11 FPS tail proxies. These are
  cumulative outlier proxies, not a conventional sliding-window 1% low.
- The user reported everything normal, including the specifically requested
  startup/focus and ordinary-play checks. No crash report or settings change
  was found. The active cache advanced to SHA-256
  `fb1e43bb9f5b37ece6c5a7342a7a994493d59d9da99b7f14f991d2a405f7d57f`;
  the old-backup cache remained unchanged.
- No paired Metal HUD numeric capture was supplied, so GPU time, app/Metal
  memory, power, and thermal comparison remain unavailable for this run.
- Evidence collection initially copied the extant legacy `/tmp` log instead of
  the production bridge log. The collector now prefers the production path;
  re-collection produced a passing swapchain verdict from the preserved run.

C1 installation checkpoint:

- After preserving B, the shared idle gate again found ESO and the launcher
  absent and Steam free of ESO file/update activity.
- The verified pristine-loader restore and corrected
  `startup-release-swapchain-control` reinstall completed with every cache and
  setting preserved.
- Installed and built bridge bytes remain identical at SHA-256
  `e429d14e3a1cc9d8e9c41e12922ecf02fe49e78e022a450d4250c503f4bee737`.
- The pre-C1 active and old-backup cache hashes are exactly B's post-run
  `fb1e43bb...f7d57f` and the unchanged `72ac0b0d...a07e1c`.

C1 gameplay checkpoint:

- Run `20260828T112013.300354000Z-pid60755` forwarded both observed swapchain
  generations unchanged and returned exactly two images for each. Promotion,
  three-image returns, capability misses, count mismatches, acquire errors, and
  present errors were all zero.
- The final summary contains 34,195 samples. Acquire p95/p99/p99.9 were
  8/12/21 us; present-call p95/p99/p99.9 were 12/16/21 us.
- Present-entry interval p95/p99/p99.9 were 18.809/21.931/37.258 ms, giving
  p99/p99.9 inverse-rate proxies of 45.60/26.84 FPS.
- No crash report or settings change was found. The active cache advanced to
  SHA-256 `64d5a0455928a4124b792b57c2360662e8dc746fd391b4efa99966588b340920`;
  the old-backup cache remained unchanged.
- The user's explicit focus/pink/perceived-FPS classification remains pending.

Provisional B-versus-C1 comparison:

- Acquire p99 is identical at 12 us. This does not support the predicted
  drawable-starvation reduction.
- Present-call p99 is identical at 16 us. B p99.9 is 24 us versus C1 21 us.
- B frame interval p95 is 18.610 ms versus C1 18.809 ms; p99 is 20.230 ms
  versus 21.931 ms (B 7.8% shorter); p99.9 is 36.880 ms versus 37.258 ms.
- B ran to the 65,536-sample cap while C1 collected 34,195 samples. Route/load
  equivalence and control noise are not established, so none of these deltas is
  yet attributed to the third image.

## Result

The corrected B and C1 runs both succeeded structurally. B's frame p99 is lower
than C1's while acquire p99 is identical, but one corrected control cannot
separate an image-count effect from route/duration noise. C2 remains pending.

## Interpretation

Confirmed: the bridge requested and used three images in two real ESO swapchain
generations, completed a long bounded measurement with no API errors, and passed
the user's focus/rendering/performance observation. The B acquire p99 of 12 us
is consistent with no material drawable wait in this arm.

Unproven: that three images caused the observed frame-p99 difference. C2 is
required to establish whether C1 is repeatable and whether B falls outside the
corrected control range. The unchanged acquire tail weakens the original
drawable-starvation mechanism.

## Rollback

The corrected two-image C1 control is installed. The verified pristine-loader
restore path remains available, and caches/settings were preserved. Its bridge
bytes are exactly the same as B; only the attested mode differs.

## Follow-up

Reinstall the same corrected control as C2 and repeat the fixed route. Compare B
only after C1-C2 establish the tail-latency range; retain B as experimental
until memory/power cost is bounded.
