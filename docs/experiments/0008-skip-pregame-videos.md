# Experiment 0008: skip pregame videos

- Date: 2026-07-21
- Outcome: **failed hypothesis; video states skipped, hot-pink frame persisted**
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

## Launcher-only update boundary: 2026-07-25

Before the pending user startup check, the launcher applied a self-update at
18:51 KST. The preserved self-update log records 308,741,853 bytes, 114 files,
134 pieces, and zero repository errors. The installed launcher still reports
version 7.1.46; its main executable SHA-256 is
`84488f45546f2b2422fb20f6f4b5fb20d9254047bf96ada2c7327321e3e356fc`.

At 18:51:38 KST, the restarted launcher's completed `Live_Prod` state check
followed `noUpdateRequired`. All eight recorded local and remote repository IDs
matched:

| Repository | ID |
|---|---|
| `PCPublicClientData` | `10eaab1417c01f916bf36f0beb68c6adf807b528` |
| `PublicCrashReporterConfig` | `3a2c0dcdc0e8b9857c1b5e94bb4644c251f62ce9` |
| `DefaultPublicPlatformsConfig` | `7913384caf70b27b3eb0278acad81183998fedd4` |
| `AppSettingsConfig` | `9d7668163e5968ef741ce0f9c5b79b4c5a0e59cb` |
| `MacPubPlayerClient` | `5e6eb98c150624a39d24ff95e0b53ba904f6741d` |
| `shared_vo_soundsets` | `ebe30cc6e934d61eeb847ad5b8076c1e843c4ff2` |
| `shared_vo_en` | `1e0af9f0ccffd65182f0615725319585066c1680` |
| `public_depot` | `f0f18bb0272831ebadb49c32073c5b3af9041242` |

No file inside the ESO game bundle had a modification time at or after the
self-update boundary. The client remains 12.0.7, databuild `3281538`; its
executable SHA-256 and UUID still match the selected target. The bridge target
is current, its marker is present, ambient occlusion remains `0`, and the
settings file still has the exact prepared SHA-256 with
`SkipPregameVideos "1"`.

The installed and cached launcher bundles both fail strict code-signature
verification, so that check cannot distinguish this update from the prior
launcher state and is not used as evidence of corruption or success. The
launcher's completed repository comparison, exact ESO identity, databuild,
bundle boundary, and unchanged experiment settings support classification as
a launcher-only update. No ESO launch occurred, so the Experiment 0008 runtime
result remains pending. Any evidence prepared before this launch-path boundary
must be replaced with fresh preflight evidence before the user test.

## Result: 2026-07-25

The user launched through the normal Steam path with
`SkipPregameVideos "1"`. The transient hot-pink frame still appeared, but
account login, character selection, and subsequent world rendering were usable.
The narrow hypothesis therefore failed.

ESO's interface log begins at `AccountLogin` and contains no
`PlayIntroMovies`, `ShowZOSVideo`, Havok, or legal-splash state. The setting
did bypass the identified pregame-video state sequence; the visual artifact
persisted outside that sequence. This narrows it to output before or during
initial swapchain/UI presentation rather than a logged logo-video state.

The user continued beyond the requested startup-only boundary and entered
Auridon. That later live resolution change is a materially different
observation and is recorded separately as Experiment 0009.

The process began at 19:07:04 KST, about two minutes before the final evidence
directory was prepared at 19:09:07. The primary time-gated startup verdict
therefore correctly found no eligible run. A retrospective check selected the
exact latest run `20260725T100704.573951000Z-pid18714` and passed: MoltenVK
1.4.1 loaded in descriptor-compatibility mode, all 17 redirects activated, the
HDR extension filter returned 130 of 131 entries, the surface filter returned
59 of 60 entries, and device creation omitted HDR. No new `.ips` report or
current-run bridge error appeared.

The 35-file ignored evidence checksum manifest has SHA-256
`7927aee697a09bf9f8e3ed32c80024d46272024d7db713d8a1215edc396d89ad`.
The exact settings before the later resolution change were reconstructed from
the preserved pre-video backup plus the verified one-line video change; its
SHA-256 exactly matches the recorded baseline
`f2e2afe947048a20ed07817930f6e442ca96da14a6ab7fe32db16b9363ec44dc`.
No rollback was performed because restoring the video state is not technically
required for the next analysis.
