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
- Run Experiment 0011 with only MTLHeap disabled. The exact gate is one
  initially valid world followed by one live resolution change and a
  30-second correctness check.
- If Experiment 0011 passes, compare steady performance and reset behavior
  against the MTLHeap-enabled checkpoint before deciding whether global heap
  disablement is acceptable or a narrower source patch is warranted.
- If it fails, restore the Experiment 0010 configuration and use
  reset-window-only tracing of offscreen images/memory, descriptors, pipelines,
  and command buffers rather than cycling another configuration flag blindly.
- Only after the reset boundary is classified, run one unchanged five-minute
  Auridon interval with ambient occlusion `0`. Use the rewritten pipeline cache
  to test, not assume, the shader-compilation explanation for stuttering.
- Defer asynchronous queue-submission experiments until the single-variable
  MTLHeap reset test is classified.

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
