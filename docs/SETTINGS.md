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
| `SkipPregameVideos` | `1` | Experiment 0008 startup-artifact test; previously `0` |
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

## Experiment 0006 SSAO safety amendment

During Experiment 0006, changing `AMBIENT_OCCLUSION_TYPE` from `0` to SSAO value
`1` coincided with a live graphics-device reset and left the game presenting
only changing solid colors. The process did not crash, but usable scene
rendering did not recover before exit.

On 2026-07-20, the exact post-SSAO file was preserved and verified before the
active value was changed from `1` back to the table's baseline `0`. The backup
hash is `0ccfd0c6d30257454d495d0c74ba6b584a46609792d357f6499ca64c81690fab`;
the active file hash after the one-line edit is
`e71a11c20828ad270c5260b648cd6adb5cd2a7a58be6677acb4a5d5ec3ed49a2`.
SSAO remains excluded from the next unchanged run. This is a one-run
compatibility warning, not a claim that SSAO is broken on every MoltenVK
configuration.

## Experiment 0008 pregame-video amendment

ESO's interface log records the repeatable solid-color symptom within the
pre-account-login sequence that begins at `PlayIntroMovies`. The executable
contains the supported `SkipPregameVideos` key, and the active value during all
observed solid-color runs was `0`.

On 2026-07-21, a guarded helper preserved the exact settings file and changed
only `SkipPregameVideos` from `0` to `1`. The pre-change hash is
`e71a11c20828ad270c5260b648cd6adb5cd2a7a58be6677acb4a5d5ec3ed49a2`;
the updated hash is
`f2e2afe947048a20ed07817930f6e442ca96da14a6ab7fe32db16b9363ec44dc`.
The result is pending a bounded startup-only run. `VIDEO_ENABLED` was not
disabled because that would broaden the change beyond the identified pregame
path.
