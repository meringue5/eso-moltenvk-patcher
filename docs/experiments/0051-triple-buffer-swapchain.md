# Experiment 0051: measured triple-buffer swapchain

- Date: 2026-08-28
- Outcome: **running; A1 measured control installed**
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
  One exit summary reports p50, p95, p99, p99.9, maxima, errors, promotion
  counts, and actual returned image counts. No per-frame log is emitted.

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

No user-controlled A1, B, or A2 result exists yet.

## Result

The A1 control is installed; controlled gameplay evidence is pending.

## Interpretation

Confirmed: the bridge can safely request and render through a three-image
MoltenVK swapchain on the exact non-game path, and can measure the intended wait
and frame-interval tails with negligible common-mode CPU cost.

Hypothesis: the additional image will reduce ESO's real drawable-starvation
tail. This remains unproven until the A1-B-A2 gameplay sequence.

## Rollback

The A1 control bridge is installed. Its only runtime difference from production
is the common swapchain timing wrapper; it forwards the two-image request.
The verified pristine-loader restore path remains available. Settings and all
pipeline-cache generations are preserved in place.

## Follow-up

The user performs A1 through the ordinary authenticated path. Do not begin B
until A1's summary, Metal HUD capture, and user observation are preserved.
