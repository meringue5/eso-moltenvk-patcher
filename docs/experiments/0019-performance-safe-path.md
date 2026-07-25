# Experiment 0019: combined performance-safe execution path

- Date: 2026-07-26
- Outcome: **installed; one user reset validation pending**
- Rollback: **Experiment 0019 installed; pristine loader and both caches checked**

## Question

Does removing the bridge's diagnostic hot path, restoring MoltenVK 1.4.1's
unmasked feature profile, and using asynchronous submission plus concurrent
pipeline compilation prevent the loaded-world reset corruption while retaining
the replacement runtime's performance?

## Why these changes are bundled

The user explicitly prioritized fewer game executions over single-variable
attribution. This candidate therefore groups changes that all remove
diagnostic or synchronous CPU-side work from the performance path:

- remove the failed Experiment 0018 feature mask;
- bypass all lifecycle observation wrappers;
- process queue submission asynchronously;
- maximize concurrent shader and pipeline compilation.

A pass will validate the bundle rather than identify which member mattered.
A failure will exclude the bundle without subdividing it into more user runs.
The more invasive `LIVE_CHECK_ALL_RESOURCES=0` descriptor-semantics change is
not included.

## Direct evidence for the hot-path cleanup

Experiment 0018 run `20260725T154920.411050000Z-pid85904` returned
`VK_SUBOPTIMAL_KHR` from all 313 generation-3 acquires and all 313 presents.
The lifecycle wrapper logs every non-success result, so its nominal eight-frame
prefix became 626 per-frame mutex, formatting, and file-flush operations after
the reset.

The wrapper observes the result after the real MoltenVK call and cannot have
created `VK_SUBOPTIMAL_KHR`. It can nevertheless perturb CPU timing and is not
appropriate in a performance candidate.

## Exact profile

```text
MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1
MVK_CONFIG_USE_MTLHEAP=1
MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0
MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1
MVK_CONFIG_USE_COMMAND_POOLING=1
MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0
```

The bridge retains official MoltenVK 1.4.1, the 17 checked static redirects,
both HDR compatibility filters, `vkCreateDevice` compatibility, and required
GIPA/GDPA routing. It does not mask physical-device features.

## Hot-path boundary

In `performance-safe` mode, proc lookup returns MoltenVK's exact original
function pointer for:

- `vkDeviceWaitIdle`;
- `vkCreateSwapchainKHR` and `vkDestroySwapchainKHR`;
- `vkGetSwapchainImagesKHR`;
- image-view, render-pass, and framebuffer create/destroy;
- `vkAcquireNextImageKHR`;
- `vkQueuePresentKHR`.

No lifecycle table, mutex, counter, formatting, or file log is reached when
those returned pointers are called. Startup validation fails closed unless
all twelve observed GDPA routes report `shim=none`, the exact effective
configuration is present, and no lifecycle record appears.

## Compatibility and risk

Asynchronous submission moves `vkQueueSubmit` and presentation processing to a
MoltenVK GCD queue. This may reduce caller-thread work and more closely resemble
the old runtime's scheduling, but it may also expose an ESO synchronization or
resource-lifetime dependency. Concurrent compilation targets pipeline hitches
and is not expected to improve steady-state GPU time.

Argument buffers remain disabled because their ESO corruption is established.
Live-resource checking remains enabled because disabling it also changes
descriptor binding semantics. MTLHeap, command pooling, and no-prefill remain
at the established performance-compatible values.

## Non-game validation

The clean shadow-bundle source build passes:

- Bink symbol re-export and Rosetta self-patch;
- HDR, feature-profile, lifecycle, reset, and render-audit smoke probes;
- all eleven MoltenVK configuration modes;
- a direct-routing unit gate covering all twelve lifecycle functions.

The exact `performance-safe` configuration query reports live resources `1`,
argument buffers `0`, MTLHeap `1`, synchronous submission `0`, command pooling
`1`, prefill `0`, and maximum concurrent compilation `1`.

On the Apple M4, the 24-cycle reset composite passes with asynchronous
submission and concurrent compilation. It alternates the two observed source
extents, command-buffer and command-pool reset, full-lifetime descriptor reuse,
render-resource recreation, submission, sampling, and Metal pixel readback.
The Metal-backed surface probe also passes: the raw 60-format table contains
the known ESO HDR pair, while the compatibility result contains 59 formats and
does not contain that pair.

All 73 Python tests, Python compilation, shell syntax, whitespace checks,
warnings-as-errors compilation, and Clang static analysis pass. Prepared
artifacts have SHA-256:

```text
source:    ffcf3e5 (Prepare Experiment 0019 performance path)
proxy:     bd6d745bd3ee218146f2ea2936d91f10cd3e55e5d42e64f95667f4d5741c287b
MoltenVK:  d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

No Steam, launcher, or ESO process was started.

## Installation gate

- Recheck the target fingerprint and all stopped processes.
- Verify the active Experiment 0018 install, pristine loader, settings, both
  pipeline caches, and candidate hashes.
- Receive explicit approval for this game-bundle modification.
- Restore the pristine loader while preserving both caches, rebuild against
  that real loader, and require byte-identical artifacts.
- Install only `performance-safe` and prepare the ignored evidence boundary.

## Installation

At approximately 2026-07-26 01:16 KST, after explicit user approval, the
installed Experiment 0018 proxy was restored to the checked pristine loader
while preserving both pipeline caches. ESO, Steam, and the launcher were
stopped. The target remained ESO 12.0.7, databuild 3281538, with executable
SHA-256 `82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`.

The bridge was rebuilt from that real loader. The rebuilt proxy, renamed
original, and official MoltenVK 1.4.1 reproduced their prepared hashes.
Installation in `performance-safe` mode then passed automatic verification:

- marker content is exactly `performance-safe`;
- installed proxy and MoltenVK are byte-identical to the rebuilt artifacts;
- installed proxy SHA-256 is
  `bd6d745bd3ee218146f2ea2936d91f10cd3e55e5d42e64f95667f4d5741c287b`;
- installed MoltenVK SHA-256 is
  `d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398`;
- pristine loader SHA-256 remains
  `c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`;
- both pipeline-cache hashes and the settings hash are unchanged;
- ESO, Steam, and the launcher remained stopped.

The ignored evidence boundary is
`artifacts/experiment-0019-20260725T161733Z`.

## User procedure

One repair validation only:

1. launch through the normal Steam path and enter the existing world;
2. change fullscreen resolution once;
3. report whether rendering remains normal, becomes black or solid, freezes,
   or crashes, then exit.

No travel, HUD, capture, FPS recording, extra option change, or repeated launch
is requested. If the known low-performance state appears, do not immediately
repeat the run.

## Pass/fail

- **Pass:** normal rendering continues after the one reset.
- **Fail:** black, solid-color, or frozen output recurs.
- **Inconclusive:** exact mode/configuration/direct-routing evidence is absent,
  the target changed, or the requested reset did not occur.

## Result

The exact candidate is installed. User validation has not started.

## Rollback

Experiment 0019 is installed. The pristine loader, both pipeline caches,
settings, displaced Experiment 0018 marker, and all prior evidence remain
preserved. The checked restore path can return to the pristine loader without
replacing either cache.
