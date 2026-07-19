# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: explain the first bridge crash

- Add deterministic per-run logging and wrap `vkCreateDevice` to record its
  enabled extension names without changing them.
- Filter `VK_EXT_hdr_metadata` during device-extension enumeration and verify in
  a non-game probe that this restores the 1.0.18 decision path without changing
  unrelated capabilities.
- Re-run wrapper/proc analysis, rebuild from source, and pass all non-game smoke
  checks before preparing another installation.
- Only after those gates pass, design a startup-only experiment whose user step
  is limited to character selection plus a 60-second wait. Define log-based
  pass/fail criteria and restore immediately after evidence collection.
- If filtering does not remove the unsafe path, return to source and binary
  analysis before considering a guarded `vkSetHdrMetadataEXT` implementation.

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
