# Roadmap

## P0: explain the first bridge crash

- Capture stderr/stdout and a backtrace before any second gameplay test.
- Determine whether the crash is a descriptor lifetime violation, a missing
  public Vulkan entry-point redirect, an ABI/configuration difference, or an
  early surface/swapchain failure.
- Test `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1` in a controlled startup-only run.
- Add opt-in Vulkan-call tracing around initialization without logging every
  gameplay call indefinitely.

## P1: make redirection exhaustive

- Enumerate all public Vulkan wrappers in the statically linked archive.
- Find call, jump, pointer-load, relocation, and table references from ESO, not
  only `E8`/`E9` direct calls.
- Prevent any old/new MoltenVK handle mixing.
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

