# Experiment 0008: skip pregame videos

- Date: 2026-07-21
- Outcome: **running; one-line setting installed, startup verification pending**
- Rollback: **not performed; exact pre-change file preserved**

## Question

Does ESO's supported `SkipPregameVideos` setting remove the repeatable
solid-color frame without changing the MoltenVK bridge or normal login UI?

## Hypothesis

The solid-color frame occurs inside the pre-UI `PlayIntroMovies` path. Setting
`SkipPregameVideos` from `0` to `1` should bypass that path and remove the
artifact. The hypothesis fails if the solid color still appears before account
login, or if skipping the videos causes a new startup or UI failure.

## Target and change set

The installed ESO 12.0.7 descriptor-compatibility bridge is unchanged. Only
this supported user-setting line changed:

```text
SET SkipPregameVideos "0" -> "1"
```

Ambient occlusion remains `0`; `VIDEO_ENABLED`, `HasPlayedPregameVideo`, and
`OpeningCinematicSeen` remain unchanged. No logo or video asset was renamed,
deleted, or replaced.

## Preflight evidence

Experiment 0007's world repeat recorded this startup sequence in ESO's own
interface log: `PlayIntroMovies`, `ShowZOSVideo`, Havok splash, legal splash,
then account login. The local executable contains the exact
`SkipPregameVideos` setting key, and the active file contained exactly one
supported line with value `0`.

A tested helper refuses ambiguous input, refuses to operate if process
inspection fails or ESO is running, preserves an exact backup, and replaces
only the supported line. Twenty-four helper, startup, and update-tool tests
passed before the live change.

At 13:26 KST, the helper preserved the pre-change file with SHA-256
`e71a11c20828ad270c5260b648cd6adb5cd2a7a58be6677acb4a5d5ec3ed49a2`
and produced SHA-256
`f2e2afe947048a20ed07817930f6e442ca96da14a6ab7fe32db16b9363ec44dc`.
Changing the one updated line back to `0` in memory reproduces the exact
pre-change hash.

## Exact user action

1. Launch through the normal Steam and launcher path.
2. Watch only from the first ESO frame through account login or character
   selection; do not enter the world or change a setting.
3. Stop immediately on a crash, hang, or corruption beyond the transient
   startup frame. Otherwise exit normally within 90 seconds.
4. Report whether the solid-color frame appeared and which, if any, ZOS,
   Havok, or legal logos remained.

The setting test passes if the solid-color frame is absent, account login or
character selection remains usable, and no crash or new persistent corruption
occurs. Bridge activation and crash evidence will be collected automatically.

## Rollback

The exact pre-change file is preserved outside Git. The tested helper supports
the inverse `1` to `0` transition if restoration becomes necessary; no rollback
is required merely to preserve an experimental state.
