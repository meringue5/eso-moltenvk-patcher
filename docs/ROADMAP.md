# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: preserve the 1.4.2 production baseline

- Treat official MoltenVK 1.4.2 plus `performance-aggressive` as the current
  production baseline. Extended ordinary play, relatively high settings, and live
  resolution/graphics resets now render correctly on the tested M4 MacBook Air.
- Preserve the 48-key sanitized standard settings asset and its source hash.
  Do not commit or distribute the complete `UserSettings.txt`.
- Do not request another dedicated graphics-reset run. Reopen that category
  only if the symptom recurs, and preserve the prior 1.4.1 failures as valid
  historical evidence rather than re-running old configuration A/B tests.
- Preserve Experiment 0022's lifecycle fact: the first 3420 x 2148 generation
  presents once and is replaced by 3420 x 2146 after 796--908 ms. Also preserve
  its screenshot amendment: exact sRGB `#FF00FF` remains visible roughly
  2.645 seconds after the corrected surface is ready, so the color is not
  confined to generation 1. Treat ESO's exact-magenta FX-material initializer
  as a supported but unconnected candidate; pregame video, native-resolution setting,
  global display/overlay, and automatic MoltenVK pink-clear explanations are
  already contradicted by local evidence.
- Preserve Experiment 0023's successful non-game discrimination mechanism,
  but do not install its invalidated generation-1-only candidate.
- Preserve Experiment 0024's completed non-game gate. Synthetic,
  real-MoltenVK/AppKit, and full-build controls pass with pixel/audit agreement,
  a generation-2 ordinal-180 finish, first-eight acquire/present logging, and a
  2,048-detail hard cap.
- Preserve Experiment 0024's exact live negative result: the user observed the
  pink frame while one generation-1 and 180 generation-2 submitted clears were
  all opaque black. There was no load-op clear. Do not repeat the clear audit.
- Static inspection shows that the opaque `ZOMetalGameView` fallback paints
  black and exposes no explicit background-color setter. Treat the known
  exact-magenta FX-material/draw path as leading, but do not call it confirmed
  until a bounded presented-pixel or draw association distinguishes it from
  other application rendering.
- Keep the redesign isolated from the normal performance path, retain the exact
  aggressive configuration, and keep the incidental build change removed.
- The one approved Steam-path startup, exact-run analysis, and restoration to
  the normal `performance-aggressive` marker are complete. Both caches were
  preserved. Do not request another user run until the next discriminating
  candidate has passed its non-game gate.
- Keep the working 1.4.2 runtime profile and cache state unchanged while
  investigating the startup artifact.
- Preserve Experiment 0025 as inconclusive and do not repeat it. The user saw
  unchanged pink, the exact hook and ordinal-180 finish were present, but the
  initializer produced zero calls inside the window, so no value was actually
  neutralized. The normal `performance-aggressive` profile is restored. A
  successor must intercept a later parameter copy/use or presented draw and
  account for any object created before the bridge constructor.
- Preserve Experiment 0026 as the completed final-swapchain result. It sampled
  five points from twenty final swapchain images immediately before present,
  requires same-queue semaphore provenance, and never changes pixels. Exact
  magenta and black controls pass at ESO's format and extents, along with the
  complete temporary bridge link and 108 Python tests. The lifecycle smoke also
  proves the exact 20-sample schedule and rejects submit-consumed semaphore
  evidence.
- The exact run completed all twenty samples while the user saw pink. Five
  points in every sampled generation-2 frame from ordinal 80 through 140 were
  exact RGBA `(255,0,255,255)` before present; earlier samples were black and
  later samples were normal scene colors. The source is inside final swapchain
  rendering, not the presentation/layer/compositor/display path. Do not repeat
  this audit.
- Correlate the black-to-magenta interval with the first graphics-pipeline and
  indexed-draw activity already visible in the proc log. Design a bounded
  successor that associates each sampled presented frame with its exact
  swapchain-linked pipeline/draw signature and shader-module hash. Reuse the
  existing pre-present pixel classification so timing shifts do not invalidate
  the correlation. Complete static, synthetic, and real non-game validation
  before requesting another user-controlled startup.
- Experiment 0027 now implements that bounded correlation and has completed its
  non-game gate. The synthetic case joins an indexed draw and complete shader
  hashes to the exact signal semaphore; the real MoltenVK/AppKit control joins
  a full-surface magenta draw to five exact pre-present magenta pixels at both
  small and ESO extents. All missing-provenance and capacity cases fail closed.
- Preserve Experiment 0027 as the completed presented-draw result. The nine
  black samples had no draw; all seven exact-magenta samples had one indexed
  draw with pipeline `c43e4410d3b33fe7`, vertex hash `c8307556011c995e`, and
  fragment hash `6907bd3576e3a930`. The same draw/pipeline continued into the
  four normal-scene samples, so do not call the shader itself a confirmed
  constant-magenta source.
- The normal `performance-aggressive` profile is restored with settings and
  backup cache preserved. Do not repeat the color, clear, or pipeline audit.
- Trace the exact pipeline's descriptor/resource inputs using static analysis,
  existing logs, and a non-game fail-closed probe first. A successor should
  distinguish a sampled image/texture from a uniform or push-value source and
  remain bounded to the already proven black-to-magenta interval. Request no
  user startup until that candidate passes its non-game gate.
- Experiment 0028 implements that successor and has passed its non-game gate.
  It compares only descriptor sets required by the exact bound pipeline,
  fingerprints the latest descriptor update batch without accumulating normal
  repeated-call counts, captures push constants, and carries the result through
  the proven submit-semaphore/present chain. Its synthetic sampled-image case,
  real MoltenVK/AppKit pipeline case, clean build, and fail-closed analyzer
  controls pass.
- Preserve Experiment 0028's exact result: the pipeline layout has one
  three-buffer set and one set with two images plus three buffers. Magenta and
  scene samples retain the same pipeline, draw, set objects, layout, and zero
  push state, while the descriptor-update signature changes. Do not repeat the
  aggregate input audit.
- The cache-preserving `performance-aggressive` restoration is complete. It
  succeeded while idle Steam remained open under the shared bundle-idle gate;
  Steam itself is not a maintenance dependency or blocker.
- Preserve the read-only pipeline-cache result: the target is ESO's final
  scene-and-GUI compositor. Its vertex set contains only three buffers; its
  fragment set contains two images and three buffers, and only the images can
  supply the observed chromatic output. Do not spend another user run on a
  generic set/image-versus-buffer split.
- Experiment 0029 implements the bounded two-input compositor audit. It orders
  the mixed-set image bindings as retained MSL `Sampler0` (scene) and
  `Sampler1` (GUI), carries image-view/base-subresource identity to present,
  samples both inputs, and distinguishes descriptor replacement from in-place
  content. Synthetic fail-closed controls, the complete build, and real
  MoltenVK/AppKit BGRA8 plus RGBA16F mip/layer controls pass. The production
  profile remains installed; one bounded startup is justified only after
  explicit installation approval.
- After identifying the offending input, prefer a startup-only neutralization
  of the application placeholder or its presentation. It must latch permanently
  back to direct forwarding at the first valid compositor input, add no
  steady-state readback, and preserve the production profile, settings, and
  caches.

### Superseded reset-investigation record

The completed items below preserve the path to the current result. They are no
longer active instructions; their detailed evidence belongs to the linked
experiment records.

- Experiment 0004 was preserved until restoration became technically necessary
  for the Experiment 0005 clean rebuild; retain its verified evidence, displaced
  marker, and cache state.
- Preserve Experiment 0005 as the verified startup-pass/rendering-fail
  checkpoint until restoration is technically required for a clean rebuild.
- Preserve the installed single-variable Experiment 0006 candidate and its
  prepared evidence. It retains both HDR filters and live-resource checking but
  disables Metal argument buffers, a capability absent from MoltenVK 1.0.18.
- Preserve the Experiment 0006 run as the first lobby-to-world rendering pass:
  the old flicker disappeared, but an unchanged five-minute interval was not
  completed before the user enabled SSAO.
- Preserve the 12.0.7 short world repeat as the target-rebase rendering pass.
  It reached Auridon for approximately 162 seconds with no perceived
  stuttering or corruption, but did not complete the planned five minutes.
- Preserve Experiment 0008 as the failed startup-artifact hypothesis: the
  setting skipped every logged video/logo state but the hot-pink frame
  persisted before account login.
- Preserve Experiment 0009 as a second live-reset rendering failure. Do not
  change resolution, SSAO, or another reset-triggering graphics option during
  an otherwise unchanged correctness or performance run.
- Preserve Experiment 0010 as a negative swapchain-path result. Its live-reset
  generation had clean tracked object lifetimes; forcing out-of-date results
  did not trigger recreation, and eliminating suboptimal presentation through
  scaling did not correct the image.
- Preserve Experiment 0011 as a negative MTLHeap result. The ESO process
  verified heap mode `0`, and a single live resolution change still produced
  solid-color output while presentation continued.
- Restore MTLHeap to `where safe` in the next candidate. Do not give up its
  resource-allocation benefit for a workaround that failed.
- Preserve Experiment 0012 as the completed reset-resource classification.
  Submitted drawing and resource creation continued without Vulkan failures;
  captured image bindings were aligned and non-overlapping, while descriptor
  and image-view churn dominated the reset boundary.
- Preserve Experiment 0013 as the negative command-pooling result. Disabling
  MoltenVK internal command pooling did not repair the reset; do not run
  another generic MoltenVK configuration A/B.
- Preserve Experiment 0014 as the completed bundled audit. Three windows had
  complete mirror coverage and found no stale set rebound, live image overlap,
  tracked layout mismatch, dead attachment, or pipeline/render-pass mismatch.
- Preserve Experiment 0015 as the negative pipeline-cache result. All 150
  reset-created graphics pipelines received `VK_NULL_HANDLE`, all 1,648
  observed reset-created pipeline binds matched their render pass, and solid
  color still recurred. Do not delete, replace, or further bypass caches as a
  reset-correctness experiment.
- Preserve Experiment 0016 as inconclusive. Three reset windows observed only
  known bound descriptor slots, valid command generations and synchronization,
  and clean tracked attachment/subresource state, but 16,816--17,888
  full-lifetime mirror overflows invalidate complete coverage.
- Do not repeat the installed full-lifetime audit. Its non-reused tombstones,
  full-array lookups, and 131,072-slot destruction scans plausibly caused the
  observed 8 FPS and gradual 30-to-8 FPS decline. Quantify that failure with a
  loaded-state non-game benchmark.
- Preserve Experiment 0017 as a negative ESO-applicability result. Its
  swizzled-view probe remains valid, but every captured ESO swapchain view was
  an identity/full-range view that bypasses the patched cache branch.
  Normal FPS returned after removing the full-lifetime audit, yet one
  resolution reset produced black output with live cursor, input, sound, and
  797 further acquire/present pairs.
- Keep Experiment 0017 installed until a source-validated successor requires
  restoration. Do not ask for another run against it.
- Preserve the completed 1.4.2 reset-repair classification: it still supplies
  no proven fix for ESO's solid-output reset. The later official-release review
  separately recommends adoption as a maintenance upgrade because the exact
  0020 compatibility and performance gates pass. Preserve its distinct
  pipeline-cache identity.
- Preserve the installed source-fixed `legacy-feature-profile` candidate from
  commit `26d26ac`. It exposes the exact embedded 1.0.18 core feature set,
  rejects any prohibited feature at device creation, retains official 1.4.1,
  and adds no hot-path audit.
- Close Experiment 0018 as a negative result. Its exact 18/18 feature mask
  still produced solid output after one resolution reset, so do not repeat or
  subdivide the core-feature experiment.
- Preserve the prepared combined `performance-safe` successor that reduces
  user runs:
  remove the failed feature mask and all lifecycle wrappers, set asynchronous
  queue submission and maximum concurrent compilation, and retain every
  established 1.4.1 compatibility setting. Keep live-resource checking
  enabled; do not fold the aggressive descriptor-semantics change into this
  bundle.
- Experiment 0019 is installed from recorded source commit `ffcf3e5`; do not
  subdivide the bundle into additional user runs.
- Close Experiment 0019 as a negative reset-repair result. Its exact
  low-overhead mode was active, yet the resolution reset still produced solid
  output. Retain it as the current performance checkpoint.
- Preserve Experiment 0020 as a successful ordinary-use
  `performance-aggressive` checkpoint.
  Its only delta from the installed safe profile is
  `LIVE_CHECK_ALL_RESOURCES=0`; a balanced M4 probe measured a 10.1% reduction
  in descriptor-heavy CPU encoding, and valid-resource reset coverage passes.
- Installation from source commit `ed5b9d3` preserved settings and both
  pipeline caches and reproduced the prepared hashes. Do not describe the
  measured encode reduction as an equal FPS increase, and do not request
  another graphics reset for this performance experiment.
- Preserve installed Experiment 0021 as the official MoltenVK 1.4.2
  maintenance baseline with the unchanged Experiment 0020 profile. The pinned
  official archive, complete bridge build, reset/device/HDR gates, balanced
  descriptor benchmark, shadow install/restore, and real post-install checks
  passed.
- Retain the versioned `db445ff2` 1.4.1 cache and exact runtime, the untouched
  pre-1.4.1 backup, and the pristine loader. Let 1.4.2 create its own
  `db660224` cache on the next ordinary launch.
- Keep shader compression as the next distinct performance candidate after
  1.4.2 ordinary use; do not silently fold it into the maintenance upgrade.

## P1: keep redirection verifiable

- Run the single quick update gate before every evidence boundary. A `READY`
  result ends routine update analysis; only a `STOP` result enters component
  diagnosis. For an unknown build, use the exact-profile fast rebase only if
  every archive, patch, reference, proc-route, and replacement-runtime check
  passes.
- When the launcher itself updates but the ESO executable remains current, use
  the completed `Live_Prod` repository comparison to distinguish a
  launcher-only event from a pending or content update. Preserve the raw logs
  and prepare fresh evidence across any launch-path boundary.
- Treat `MANUAL_ANALYSIS_REQUIRED` as a hard stop; extend the analyzer instead
  of weakening or bypassing a mismatched profile.
- Fail closed if any externally referenced old wrapper lacks a new-runtime
  export or any proc query regresses from non-null to null on its actual route.
- Investigate any newly discovered path that could mix old and new runtime
  ownership before changing the redirect set.
- Add a startup smoke mode that stops before account login or world entry.

## P2: deferred controlled performance experiments

- Do not request manual performance telemetry while non-game M4 measurements
  can still classify the remaining runtime settings.
- Treat the automated descriptor-encode measurement and Experiment 0020 install
  as complete. Validate it through ordinary use rather than a dedicated
  graphics-reset run.
- Retain Experiment 0003 only as the embedded-runtime settings and warm-cache
  checkpoint; do not attribute its qualitative FPS change to MoltenVK 1.4.1.
- When correctness is established, define a fixed-zone, fixed-route A/B with a
  maximum duration, event-based stop condition, exact metrics, and pass/fail
  criteria before asking the user to play.
- Compare MoltenVK defaults against live-resource compatibility mode in the
  non-game probe first. Experiment 0019 already verified asynchronous queue
  submission in ESO without an obvious non-rendering regression.
- Keep command pooling at its 1.4.1 default. Metal argument buffers now require
  a correctness A/B because Experiment 0005 exposed a descriptor-like visual
  failure and the embedded runtime predates that feature.
- Avoid command-buffer prefilling until reset/reuse behavior is known; it can
  increase memory and create artifacts for incompatible command-buffer usage.
- Classify shader-source compression next with a non-game M4 probe. Compare
  `LZ4` against no compression first, measuring retained MSL-source memory,
  compression/decompression time, and cold and cache-backed pipeline creation
  latency after the 1.4.2 maintenance candidate is prepared. It is a
  memory-pressure candidate, not an assumed shader-execution or FPS
  optimization. Install it only if measured memory savings outweigh its
  compilation and cache-load costs.

## P3: recovery without logout

- Identify whether a zone reload, UI reload, graphics-device reset, or cache
  trim releases the accumulated state.
- Search for safe engine commands or API paths before considering invasive
  runtime state destruction.
