# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: explain the first bridge crash

- Experiment 0004 was preserved until restoration became technically necessary
  for the Experiment 0005 clean rebuild; retain its verified evidence, displaced
  marker, and cache state.
- Collect Experiment 0005 after its approved startup-only run and classify the
  automatic bridge verdict together with the user's 60-second observation.
- If the exact surface-format filter prevents the setter query and reaches a
  stable character selection, design a separate short stability experiment.
  Do not treat that pass as gameplay or performance validation.
- If it fails, use the new run-scoped query and crash evidence before considering
  a guarded setter. Do not add a no-op merely because the proc is NULL.

## P1: keep redirection verifiable

- Re-run wrapper and GIPA/GDPA analysis after every target executable update.
- Fail closed if any externally referenced old wrapper lacks a new-runtime
  export or any proc query regresses from non-null to null on its actual route.
- Investigate any newly discovered path that could mix old and new runtime
  ownership before changing the redirect set.
- Add a startup smoke mode that stops before account login or world entry.

## P2: deferred controlled performance experiments

- Do not request gameplay or performance telemetry until a startup experiment
  proves that MoltenVK 1.4.1 is active and stable.
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
- Keep Metal argument buffers and command pooling at their 1.4.1 defaults first.
- Avoid command-buffer prefilling until reset/reuse behavior is known; it can
  increase memory and create artifacts for incompatible command-buffer usage.

## P3: recovery without logout

- Identify whether a zone reload, UI reload, graphics-device reset, or cache
  trim releases the accumulated state.
- Search for safe engine commands or API paths before considering invasive
  runtime state destruction.
