# Experiment 0011: MTLHeap disabled during live reset

- Date: 2026-07-25
- Outcome: **planned**
- Rollback: **not performed; Experiment 0010 checkpoint currently installed**

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

## Exact user action

Pending successful installation:

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

Pending.
