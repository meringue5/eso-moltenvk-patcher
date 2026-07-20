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
- Repeat the installed build unchanged in the same area for five minutes. Use
  the now-restored `AMBIENT_OCCLUSION_TYPE "0"` baseline, warm pipeline cache,
  and compiler-warning timing to test, not assume, the shader-compilation
  explanation for stuttering.
- Instrument pipeline creation and the graphics-device reset before a separate
  SSAO experiment. Compare clean-start SSAO with a live SSAO toggle; do not
  change a MoltenVK control in that comparison.
- Defer MTLHeap and asynchronous queue-submission experiments. Argument buffers
  improved the original artifact, and the new SSAO boundary must be isolated
  first.

## P1: keep redirection verifiable

- Re-run wrapper and GIPA/GDPA analysis after every target executable update.
- Fail closed if any externally referenced old wrapper lacks a new-runtime
  export or any proc query regresses from non-null to null on its actual route.
- Investigate any newly discovered path that could mix old and new runtime
  ownership before changing the redirect set.
- Add a startup smoke mode that stops before account login or world entry.

## P2: deferred controlled performance experiments

- Do not request performance telemetry until the unchanged five-minute
  Experiment 0006 repeat establishes short gameplay stability.
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
