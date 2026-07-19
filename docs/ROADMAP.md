# Roadmap

This document is forward-looking. The current safety state and active blocker
are in [Project status](STATUS.md); completed runs belong in the
[experiment index](experiments/README.md).

## P0: explain the first bridge crash

- Run Experiment 0002 once in startup-only `live-check` mode with GIPA and GDPA
  tracing, prepared `.ips` capture, and unified-log collection.
- If no null proc result precedes the same fault, trace the ESO graphics
  abstraction vtable and initialization callbacks around the recovered
  indirect call.
- Distinguish descriptor lifetime, ABI/configuration, and early
  surface/swapchain failure before adding broad Vulkan-call tracing.

## P1: keep redirection verifiable

- Re-run wrapper and GIPA/GDPA analysis after every target executable update.
- Fail closed if any externally referenced old wrapper lacks a new-runtime
  export or any proc query regresses from non-null to null on its actual route.
- Investigate any newly discovered path that could mix old and new runtime
  ownership before changing the redirect set.
- Add a startup smoke mode that stops before account login or world entry.

## P2: controlled performance experiments

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
