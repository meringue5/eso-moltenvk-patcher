# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: preserve the validated 1.4.2 gameplay checkpoint

- Treat official MoltenVK 1.4.2 plus `performance-aggressive` as the current
  tested baseline. Extended ordinary play, relatively high settings, and live
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
- Experiment 0025 now supplies the next discriminating candidate. It validates
  and hooks the exact `FXMaterial` initializer only in the two-generation audit
  window, runs the original initializer, and changes only three exact magenta
  vectors to black while preserving alpha. Its synthetic trampoline, bounded
  window, full build, configuration, and 104-test gates pass. The explicitly
  approved `startup-fx-neutralize` installation now matches the build, while
  both caches and settings retain their pre-install hashes.
- One matched, bounded startup decides this branch: disappearance/change to
  black confirms the sentinel as causal; unchanged pink excludes it. Missing
  initializer evidence or a missing ordinal-180 finish is inconclusive. After
  evidence collection, restore `performance-aggressive` and preserve both
  caches and settings.
- Preserve Experiment 0025 as inconclusive and do not repeat it. The user saw
  unchanged pink, the exact hook and ordinal-180 finish were present, but the
  initializer produced zero calls inside the window, so no value was actually
  neutralized. The normal `performance-aggressive` profile is restored. A
  successor must intercept a later parameter copy/use or presented draw and
  account for any object created before the bridge constructor.
- Experiment 0026 is the completed non-game successor candidate. It samples
  five points from twenty final swapchain images immediately before present,
  requires same-queue semaphore provenance, and never changes pixels. Exact
  magenta and black controls pass at ESO's format and extents, along with the
  complete temporary bridge link and 108 Python tests. The lifecycle smoke also
  proves the exact 20-sample schedule and rejects submit-consumed semaphore
  evidence.
- The explicit `startup-present-pixel-audit` approval, cache-preserving restore,
  clean source build, exact-mode install, and byte/hash verification are
  complete. The installed diagnostic retains both caches and settings.
- Refresh the stale launcher state through the user-controlled normal Steam
  launcher without pressing Play. Do not weaken the 3,600-second evidence gate.
  Once a fresh evidence start boundary is prepared, one user-controlled normal
  Steam-path startup decides whether the remaining source is inside final
  swapchain content or after that boundary. Missing any of twenty samples is
  inconclusive.

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

## P4: public distribution without a Steam-only installation workflow

This is packaging and compatibility work, not a change to the validated bridge
or an authorization to install it on another client.

- Deliver a signed and notarized installer/manager app in a GitHub Release disk
  image. It must bundle the verified bridge and replacement runtime so normal
  users do not need Python, Xcode, a source checkout, or shell commands.
- Do not present `teso4m4` as an ESO add-on. The bridge belongs beside the
  game's executable and cannot work from the `AddOns` folder.
- Make the installer edition-neutral: discover known Steam and ZeniMax launcher
  locations, allow explicit selection for custom locations, then identify the
  contained `eso.app` by validated structure and binary identity instead of by
  its enclosing path.
- Maintain a supported-client profile registry keyed by exact executable
  identity and static-layout checks. An unrecognized build or unverified
  distribution must fail closed.
- Keep launch and account authentication outside the installer. Players must
  continue using their normal Steam or official ESO launcher flow.
- Make backup, status, repair, and removal first-class installer actions. Store
  enough per-installation state to restore only the selected client safely.
- Require ESO and its launcher to be closed before changing selected game
  files. Do not require Steam to quit merely as a blanket rule; diagnose actual
  target-file access and prompt only for the process that blocks it.
- Before claiming direct-launcher support, acquire a current lawful
  direct-launcher client and complete identity comparison, install/restore,
  launcher update, and normal-authentication validation. Reuse the Steam
  profile only when the complete validation matches; otherwise add a separate
  profile.
