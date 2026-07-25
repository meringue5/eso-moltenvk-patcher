# Experiment 0009: live resolution reset

- Date: 2026-07-25
- Outcome: **failed rendering correctness; solid-color output after live reset**
- Rollback: **not performed; post-run settings and prior exact state preserved**

## Context

This was an unplanned continuation of Experiment 0008 rather than a requested
controlled test. After startup, character selection, and Auridon rendered
normally, the user lowered the fullscreen resolution in ESO's live graphics
menu. Output then became unusable solid colors and did not recover, so the user
force-terminated the game.

## Exact settings transition

A structural settings comparison reconstructed the exact pre-run file from the
preserved backup and verified its SHA-256 as
`f2e2afe947048a20ed07817930f6e442ca96da14a6ab7fe32db16b9363ec44dc`.
The post-run file has SHA-256
`5912a842e7157a73d189f43a1faaf6d8a7635b7cae8a6c0920afff417ae6b21a`.
The file structure and setting-key set are unchanged. Four values differ:

```text
SET FullscreenWidth "2048" -> "1920"
SET FullscreenHeight "1280" -> "1200"
SET WeaponsOutCameraZoomDistance "3.00000000" -> "2.00000000"
SET WeaponsSheathedCameraZoomDistance "3.00000000" -> "2.00000000"
```

The two camera-distance changes are incidental user input. The reported visual
failure immediately followed the explicit resolution transition while
`FULLSCREEN "2"`, `SUB_SAMPLING "1"`, ambient occlusion `0`, and
`SkipPregameVideos "1"` remained unchanged.

## Runtime evidence

The bridge run began at 19:07:04 KST and passed retrospective startup
validation. ESO's interface log reached a fully loaded Auridon at 19:07:40.
At 19:10:09, immediately before the reported corruption, the client log
recorded:

1. `DeviceWaitIdle` taking 13.612667 ms;
2. a second `DeviceWaitIdle`;
3. `fpCreateSwapchainKHR`;
4. `OnDeviceReset`.

The bridge trace records swapchain and dependent-resource destruction,
surface-format filtering during recreation, new shader/pipeline setup, and
eventual orderly Vulkan object teardown. It contains no current-run bridge
error, HDR setter lookup, or recorded device loss. No new `.ips` report was
created despite the user-visible hang and force termination.

The active pipeline cache grew from 3,983,422 to 4,041,740 bytes and changed
content. That is evidence of new retained pipeline data only; it does not
identify the corrupt pipeline or prove a cache cause.

## Interpretation

### Confirmed

- lowering the live fullscreen resolution coincided with a swapchain/device
  reset and immediate persistent solid-color output;
- the same bridge configuration rendered startup, character selection, and the
  world correctly before that transition;
- the failure was not a process crash, Vulkan device-loss report, HDR setter
  call, or bridge activation failure.

### Inference

Experiment 0006's live SSAO transition produced the same user-visible
solid-color class after the same high-level
`DeviceWaitIdle -> swapchain recreation -> OnDeviceReset` sequence. A
resolution-only trigger does not require SSAO, so the common loaded-world reset
and resource-recreation path is now better supported than an SSAO-specific
shader explanation. This does not yet identify whether ESO or MoltenVK retains,
destroys, or rebuilds the wrong resource.

### Next gate

Do not request another live graphics-option change yet. First instrument
swapchain and dependent-resource lifecycle calls around `OnDeviceReset`,
including create/destroy results and the first presentations after recreation.
A later cold start at the preserved 1920 x 1200 setting can distinguish a bad
live transition from an intrinsically unsupported resolution, but only after
that instrumentation is ready.
