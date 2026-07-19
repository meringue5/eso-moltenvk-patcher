# Settings record

This document separates observed behavior from unverified internet advice.

## Current sanitized snapshot

The relevant values preserved after the Experiment 0003 long baseline session
are stored in
[`config/usersettings-performance-observed.txt`](../config/usersettings-performance-observed.txt).

Notable values:

| Setting | Value | Status |
|---|---:|---|
| `OCCLUSION_CULLING_ENABLED` | `1` | Accepted by ESO; no isolated FPS A/B yet |
| `CHARACTER_RESOLUTION` | `1` | Preserves player/character clarity |
| `SUB_SAMPLING` | `1` | Preferred visually; lowering gives a large but unacceptable quality loss |
| `VIEW_DISTANCE` | `1.03999996` | Current value; ESO later overwrote a manual `0.80000001` test |
| `PFX_SUPPRESS_DISTANCE` | `35` | Current reduced effect distance |
| `REFLECTION_QUALITY` | `0` | Reduced |
| `PLANAR_WATER_REFLECTION_QUALITY` | `0` | Disabled/reduced |
| `SCREENSPACE_WATER_REFLECTION_QUALITY` | `1` | Still enabled at a low setting |
| `SHADOWS` | `2` | Current checkpoint value |
| `ANTIALIASING_TYPE` | `0` | Disabled in the current checkpoint |
| `AMBIENT_OCCLUSION_TYPE` | `0` | Disabled |
| `GOD_RAYS` / `BLOOM` | `0` | Disabled |
| `VSYNC` | `1` | Enabled |
| `MinFrameTime.2` | `0.01000000` | 100 FPS interval, not a general performance unlock |
| `MaxCoresToUse.4` | `-1` | Automatic; manually entering M4 core count is not validated |

## Findings about common advice

- `-rdevice vk` is not a useful renderer switch here. ESO already uses Vulkan
  wrappers translated through statically linked MoltenVK.
- `MaxCoresToUse` should not be changed merely to match the advertised CPU core
  count. `-1` lets the engine choose; no evidence showed that a manual value
  fixes the town/player-count slowdown.
- `MinFrameTime.2` controls a frame interval/cap. It does not remove GPU work.
- Lower subsampling is the largest easy FPS gain, but it visibly lowers the
  entire rendered scene and was rejected for normal play.
- Lower character resolution also affects the local character and nearby
  character presentation, so it is a poor first response to player-density
  slowdown.
- True fullscreen was observed to activate macOS Game Mode. Windowed fullscreen
  is easier to switch away from but did not provide that benefit in this setup.
- Internal `VIEW_DISTANCE` values should not be assumed to equal the visible
  in-game slider numerically. ESO rewrote the manually tested `0.80` value after
  later in-game changes.

The current full settings file and 6.8 MB pipeline cache are retained only in
ignored Experiment 0003 evidence. Their committed counterpart is this sanitized
setting subset; neither the raw settings nor the cache belongs in Git.
