# Settings record

This document separates observed behavior from unverified internet advice.

## Current M4/MoltenVK 1.4.2 standard

The current sanitized template is
[`config/usersettings-m4-moltenvk-1.4.2-standard.txt`](../config/usersettings-m4-moltenvk-1.4.2-standard.txt).
It contains 48 allowlisted graphics, display, and performance keys copied
exactly from the stable 2026-08-01 checkpoint. It deliberately excludes
account, input, audio, UI, add-on, and machine-specific state and is not a
complete replacement for `UserSettings.txt`.

The public release bundles this template as an explicit install option. There
initially highlights Apply: the player may confirm it or move to Skip. Application
backs up the complete live file and selectively merges these 48 keys. Removal
restores the backup only when the live file still matches the applied result,
so settings changed after installation are not overwritten.

Validated context:

| Item | Value |
|---|---:|
| Hardware | Apple M4 MacBook Air |
| ESO | 12.0.7, databuild 3281538 |
| Runtime | official MoltenVK 1.4.2 |
| Bridge mode | `performance-aggressive` |
| Display | exclusive fullscreen, 2048 x 1280 |
| Character resolution | `2` |
| Subsampling | `1` |
| Shadows / high-resolution shadows | `2` / `1` |
| Planar / screen-space water reflections | `2` / `1` |
| Ambient occlusion | `1` |
| View distance | `1.03999996` |
| VSync | `1` |

The source full-file SHA-256 is
`470c9acaa599b61fabe8759c0089c69e31fb9723b34326c3949cd82db6a76382`.
The user reports normal rendering after live resolution and graphics-setting
changes and comfortable gameplay at these relatively high settings. The latest
client log independently confirms six completed reset sequences with zero
error markers; visual correctness after those resets is the user's observation.

The settings template and `SkipPregameVideos=1` do not themselves repair the
formerly visible hot-pink startup frame. Experiment 0031 now neutralizes that
frame in the bridge with an exact, bounded compositor substitution while
leaving this settings file unchanged. The template remains a settings record,
not the mechanism for the startup presentation repair.

## Historical Experiment 0003 snapshot

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
| `SkipPregameVideos` | `1` | Skips logged video states; does not remove hot-pink startup frame |
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

The historical full settings file and 6.8 MB pipeline cache are retained only in
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
On 2026-07-25, the interface log began directly at `AccountLogin` and omitted
the identified video/logo states, but the user still observed the hot-pink
frame. The setting works as a video bypass and is not a fix for the artifact.
`VIDEO_ENABLED` remains unchanged.

## Experiment 0009 live-resolution safety amendment

Changing the loaded-world fullscreen resolution from 2048 x 1280 to
1920 x 1200 immediately preceded a persistent solid-color display. The client
recorded `DeviceWaitIdle`, swapchain recreation, and `OnDeviceReset`, matching
the high-level sequence seen after the Experiment 0006 SSAO toggle.

The lower resolution remains in the active file and has not yet been validated
from a cold start. Until reset instrumentation is ready, do not change
resolution, ambient occlusion, or other reset-triggering graphics options
during a correctness or performance run.

## 2026-08-01 MoltenVK 1.4.2 amendment

The Experiment 0006 and 0009 warnings above remain accurate for their MoltenVK
1.4.1 checkpoints but are no longer the current operational restriction. With
official MoltenVK 1.4.2, the unchanged `performance-aggressive` bridge profile,
and the new 1.4.2 cache identity, the user has changed graphics settings and
resolution without persistent solid-color or frozen-frame output. Ambient
occlusion is now `1`, fullscreen resolution is 2048 x 1280, and subsequent
play remained normal.

This later observation does not prove which internal change repaired the
behavior. It does establish the committed 1.4.2 template as the current tested
standard and removes the old “do not change graphics settings” restriction for
this exact checkpoint.
