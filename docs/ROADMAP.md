# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: explain the first bridge crash

- Record the exact device-extension names ESO passes to `vkCreateDevice` and
  reproduce that advertised-versus-enabled state in the non-game probe.
- Determine whether hiding `VK_EXT_hdr_metadata` during extension enumeration
  restores the 1.0.18 decision path without changing unrelated capabilities.
- Compare that filtering approach with a guarded compatibility implementation
  of `vkSetHdrMetadataEXT`; prefer the smallest behavior that matches the old
  runtime and fails closed on an unexpected device state.
- Design a third startup-only experiment only after the negotiation mismatch is
  reproduced without launching the game.

## P1: keep redirection verifiable

- Re-run wrapper and GIPA/GDPA analysis after every target executable update.
- Fail closed if any externally referenced old wrapper lacks a new-runtime
  export or any proc query regresses from non-null to null on its actual route.
- Investigate any newly discovered path that could mix old and new runtime
  ownership before changing the redirect set.
- Add a startup smoke mode that stops before account login or world entry.

## P2: controlled performance experiments

- Use the Experiment 0003 embedded-runtime checkpoint for a fixed-zone,
  fixed-route repeat before attributing any change to MoltenVK 1.4.1.
- Capture paired Metal HUD states during the object-heavy 40-FPS condition and
  after spontaneous recovery: FPS, GPU time, frame interval, app memory, Metal
  memory, and thermal state.
- Record route duration, camera, resolution, player-density estimate, settings
  snapshot, and pipeline-cache hash. Preserve the warm cache before any
  cold-cache comparison; never delete it.
- Use GPU time versus frame interval to distinguish a GPU-work increase from a
  CPU/submission/streaming stall before changing another option.
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
