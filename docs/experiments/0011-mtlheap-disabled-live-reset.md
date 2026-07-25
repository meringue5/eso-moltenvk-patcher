# Experiment 0011: MTLHeap disabled during live reset

- Date: 2026-07-25
- Outcome: **failed; MTLHeap disabled but live reset still produced solid-color
  output**
- Rollback: **not performed; restore path checked before installation**

## Question

Does disabling MoltenVK's Metal-heap allocation path prevent the loaded-world
visual corruption after an ESO graphics-device reset?

## Basis and hypothesis

Experiment 0010 found clean swapchain-dependent object lifecycles and proved
that eliminating `VK_SUBOPTIMAL_KHR` does not restore valid output. Simulation,
input, and audio continue while the presented content becomes a frozen frame
or solid color. The defect is therefore downstream of ESO's reset initiation
but upstream of successful presentation.

MoltenVK 1.4.1 uses `MTLHeap` where safe by default, which is effectively the
active heap path on Apple GPUs. The embedded MoltenVK 1.0.18 runtime predates
that implementation. Live graphics changes recreate loaded-world resources,
making heap placement or reuse the narrowest remaining single configuration
delta.

The hypothesis passes if one live resolution change completes and the world
continues rendering correctly. It fails on a frozen last frame, persistent
solid color, flicker, hang, or crash.

## Exact change and controls

- Keep official MoltenVK 1.4.1.
- Keep all 17 byte-validated redirects.
- Keep live-resource checking enabled.
- Keep Metal argument buffers disabled.
- Change only `MVK_CONFIG_USE_MTLHEAP` from `1` (`where safe`) to `0`
  (`never`).
- Keep synchronous queue submission enabled, command pooling enabled, and
  command-buffer prefill disabled.
- Keep both HDR compatibility filters and observation-only lifecycle tracing.
- Preserve user settings and both pipeline caches.
- Use the distinct, fail-closed marker mode `legacy-allocation`.

The bridge queries `vkGetMoltenVKConfigurationMVK` after loading the runtime
and refuses to patch unless the effective values exactly match this mode.

## Non-game gate

Before installation:

1. verify the current ESO fingerprint and launcher-update state;
2. preserve Experiment 0010's combined post-mortem log;
3. restore only the pristine loader while retaining both pipeline caches;
4. rebuild from the new source commit;
5. verify default, descriptor-compatible, and legacy-allocation configuration
   states in separate processes;
6. run proxy, self-patch, HDR, lifecycle, Vulkan, static, Python, and shell
   checks;
7. install in `legacy-allocation` mode and verify installed hashes and marker.

No agent launches Steam, the launcher, or ESO.

## Installation checkpoint

Source commit `a354d9e` passed 46 Python tests, Python compilation, shell syntax,
whitespace checks, Clang warnings-as-errors, and Clang static analysis. The
clean rebuild passed Bink re-export, Rosetta self-patch, HDR filter, lifecycle
forwarding, and all three independent MoltenVK configuration probes:

| Mode | Live resources | Argument buffers | MTLHeap | Sync submit | Command pooling | Prefill |
|---|---:|---:|---:|---:|---:|---:|
| default | 0 | 1 | 1 | 1 | 1 | 0 |
| `descriptor-compat` | 1 | 0 | 1 | 1 | 1 | 0 |
| `legacy-allocation` | 1 | 0 | 0 | 1 | 1 | 0 |

Fresh legacy and replacement-runtime probes reached the real M4 Metal device.
The 1.4.1 candidate hid the HDR device extension, removed the one exact HDR
surface pair, created a non-HDR device, and reported the intended discrete
descriptor-binding path. No game process was launched.

The pristine loader was restored only as a clean-build prerequisite, with both
pipeline caches preserved. Installation then completed in
`legacy-allocation` mode. The installed and built hashes match:

```text
libBink2Macx64.dylib
9aaf37b0b60575de0ad0535e343f24462e4f8dca2823f04960f34395b23569eb

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

Post-install status reports the recognized ESO target, current bridge, and
present marker containing exactly `legacy-allocation`. The quick update gate is
`READY`. Evidence is prepared under the ignored
`artifacts/experiment-0011-20260725T111616Z` directory.

The active user settings SHA-256 is
`f579755ad6da18be3e52a33481a16d30e64a21881db0b459d00f63e5197b395f`;
ambient occlusion is currently `1` and pregame video skipping is `1`. The
4,190,143-byte active pipeline cache has SHA-256
`ed509d7c359e883115bc7db8aa85fccf494cbee061e8b77cb6de118974b37db9`.
The old 6,800,792-byte backup remains unchanged at SHA-256
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

## Exact user action

The installation gate passed. Perform exactly one user-controlled run:

1. Launch through Steam and enter the existing character's world.
2. Confirm the initial world is rendering normally.
3. Change fullscreen resolution once to any different available value, then
   apply it.
4. Stop immediately if the image freezes, becomes a persistent solid color,
   flickers, hangs, or crashes.
5. If rendering remains correct, move and rotate the camera for 30 seconds,
   then exit normally.

No Metal HUD, screenshot, FPS capture, travel, or second setting change is
required. The user need report only initial rendering pass/fail and post-change
rendering pass/fail.

## Result

The user launched through Steam, reached the world, and changed fullscreen
resolution once. The display became persistent solid color immediately after
the change. The process did not crash.

Run `20260725T111738.719260000Z-pid33237` passed the automatic bridge verdict.
The ESO process itself reported the exact effective configuration:

```text
live_resources=1 metal_argument_buffers=0 use_mtlheap=0
synchronous_queue_submits=1 command_pooling=1 prefill=0
```

The settings comparison has exact structural identity and only two changed
values:

```text
FullscreenWidth  1920 -> 2048
FullscreenHeight 1200 -> 1280
```

ESO recorded two startup resets and one terminal live reset. The live reset
began with a 14.874 ms device wait, recreated the swapchain in 0.076 ms, and
completed `OnDeviceReset` in 0.262 ms. The bridge observed three generations:

| Generation | Extent | Images | Views | Framebuffers | Acquires | Presents |
|---:|---|---:|---:|---:|---:|---:|
| 1 | 3420x2148 | 2 | 2/2 | 2/2 | 1 | 1 |
| 2 | 3420x2146 | 2 | 2/2 | 2/2 | 8 | 8 |
| 3 | 3420x2146 | 2 | 2/2 | 2/2 | 382 | 382 |

All 382 generation-3 acquisitions and presentations returned
`VK_SUBOPTIMAL_KHR`. There was no lifecycle anomaly, bridge error, device loss,
new crash report, or OS focus loss. The active cache retained its size and
changed content; the old backup remained byte-identical. All 38 evidence files
pass the ignored directory's checksum manifest.

## Interpretation

The MTLHeap hypothesis is falsified for this failure. Disabling Metal-heap
allocation does not repair ESO's loaded-world reset path, so MTLHeap must not
be traded away as a workaround for this defect.

Together with Experiment 0010, the result excludes the tested swapchain result,
presentation scaling, and MTLHeap allocation paths. The invalid content is
created or composed before a functioning presentation. The next candidate must
observe the reset window's non-swapchain images and memory bindings,
descriptors, pipelines, command buffers, and queue submissions without changing
their inputs or results.
