# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: restore rendering correctness after the startup fix

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
- Preserve the completed 1.4.2 classification. Its core features and limits
  match 1.4.1 under the bridge configuration, its reset-relevant known changes
  do not match ESO's captured extension/image/query paths, and both versions
  pass the 24-cycle reset composite. Do not install 1.4.2 without a new
  differential reason, and preserve its distinct pipeline-cache identity.
- Install only the source-fixed `legacy-feature-profile` candidate from
  commit `26d26ac` after explicit approval. It must expose
  the exact embedded 1.0.18 core feature set, reject any prohibited feature at
  device creation, retain official 1.4.1, and add no hot-path audit.
- Keep the next user-controlled execution for a prepared repair validation:
  one normal-performance world entry and one graphics reset, with no additional
  diagnostic-only run first.
- Only after the reset boundary is classified, run one unchanged five-minute
  Auridon interval with ambient occlusion `0`. Use the rewritten pipeline cache
  to test, not assume, the shader-compilation explanation for stuttering.
- Defer asynchronous queue-submission experiments until the reset resource
  trace is classified.

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

- Do not request performance telemetry until an unchanged five-minute
  Experiment 0007 interval extends the two short gameplay passes.
- Before requesting a performance run, establish an automated measurement path
  that does not depend on the currently unavailable Metal HUD or manual image
  capture.
- Retain Experiment 0003 only as the embedded-runtime settings and warm-cache
  checkpoint; do not attribute its qualitative FPS change to MoltenVK 1.4.1.
- When correctness is established, define a fixed-zone, fixed-route A/B with a
  maximum duration, event-based stop condition, exact metrics, and pass/fail
  criteria before asking the user to play.
- Compare MoltenVK defaults against live-resource compatibility mode.
- Evaluate asynchronous queue submission only after correctness is established.
- Keep command pooling at its 1.4.1 default. Metal argument buffers now require
  a correctness A/B because Experiment 0005 exposed a descriptor-like visual
  failure and the embedded runtime predates that feature.
- Avoid command-buffer prefilling until reset/reuse behavior is known; it can
  increase memory and create artifacts for incompatible command-buffer usage.

## P3: recovery without logout

- Identify whether a zone reload, UI reload, graphics-device reset, or cache
  trim releases the accumulated state.
- Search for safe engine commands or API paths before considering invasive
  runtime state destruction.
