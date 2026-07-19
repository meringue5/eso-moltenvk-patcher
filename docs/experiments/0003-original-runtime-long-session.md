# Experiment 0003: original-runtime long-session checkpoint

- Date: 2026-07-19
- Outcome: **inconclusive as an override test; embedded-runtime baseline captured**
- Rollback: **not applicable; the original loader remained active**

## Question

Did the reported stable, higher-FPS session run through the MoltenVK 1.4.1
bridge, and what state can be retained as a performance checkpoint?

## Hypothesis

If MoltenVK 1.4.1 was active, the session's loaded-image list would contain the
teso4m4 Bink proxy, its pristine Bink re-export target, and
`libMoltenVK.teso4m4.dylib`. The bridge log would also contain a new activation
record from the session.

## Target and change set

- ESO SHA-256:
  `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`
- ESO Mach-O UUID: `867e93bc-a6e7-3109-bf8e-542ff59ccdff`
- No teso4m4 installation was active after Experiment 0002 rollback.
- The current sanitized graphics settings are recorded in
  [`config/usersettings-performance-observed.txt`](../../config/usersettings-performance-observed.txt).
- Rust was installed before this analysis: `rustc 1.97.1`, `cargo 1.97.1`,
  with `aarch64-apple-darwin` and `x86_64-apple-darwin` targets.

## Preflight limitation

This was a user-reported session examined retrospectively, not a prepared A/B
run. There was no before-session cache hash, fixed route, fixed player density,
timed capture interval, or paired Metal HUD capture. It can establish which
runtime was loaded and preserve the end state, but cannot establish the cause
or exact magnitude of the reported FPS change.

## Procedure

The user launched ESO through the normal Steam and launcher path, played from
15:49:03 to 18:16:20 KST, exited normally from the user's perspective, and then
reported the observed performance. The launcher subsequently presented a
crash report. The agent did not launch ESO, Steam, or the launcher.

After the report, the active-loader status, bridge-log modification time, new
`.ips`, loaded images, current settings, pipeline cache, and Rust targets were
inspected read-only. The raw end-state files were copied to ignored evidence
storage.

## Evidence

Raw evidence is preserved under ignored directory
`artifacts/experiment-0003-20260719T181624KST/`. It contains the new `.ips`,
full settings files, pipeline cache, status snapshot, and the unchanged bridge
log. These raw files must not be committed.

The directory includes `SHA256SUMS`; `shasum -c SHA256SUMS` passed for all
eight captured files.

| Evidence | Observation |
|---|---|
| Process lifetime | `15:49:03.537` to `18:16:20.504` KST, about 2 h 27 min |
| Bridge log | Last modified at 15:22:47 KST, before this process launched |
| Loaded Bink image | Original UUID `0a18bf3d-361f-382a-bd3b-83f2b29d14fb` |
| Bridge/MoltenVK images | No proxy or `libMoltenVK.teso4m4.dylib` image present |
| Active Bink SHA-256 | `c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb` |
| Pristine Bink SHA-256 | Same as active Bink |
| Exit `.ips` SHA-256 | `ebf80afd5fb6976a1f5635e1824eea55c9651a712faf3b235a887eb44ceccd12` |
| Settings SHA-256 | `60c604b0fc4e629291c436f492f7966cbf7334e1b201a48062554bd94dcf7267` |
| Pipeline cache | 6,800,792 bytes; SHA-256 `72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c` |

The `.ips` records `EXC_BAD_INSTRUCTION / SIGILL` on a worker thread during
CoreAudio/AudioToolbox teardown. Its stack includes
`ExtendedAudioBufferList_Destroy`, `AudioComponentInstanceDispose`, and ESO
audio cleanup frames. This differs from Experiments 0001 and 0002, which
crashed during startup with `EXC_BAD_ACCESS`, `RIP=0`, and MoltenVK 1.4.1
loaded.

User-reported performance observations, without automated measurements:

- General FPS appeared more than 10 FPS higher than the earlier 1.0-era
  experience.
- Object-heavy map areas still dropped into the 40s.
- Changing graphics options in that state did not noticeably raise FPS.
- While continuing through a town at about 40 FPS, the session sometimes
  recovered to about 60 FPS without logout, unlike the earlier observed
  recovery that required leaving and re-entering the world.

## Result

The MoltenVK 1.4.1 override was not active. The loaded-image evidence and
unchanged bridge log falsify the override hypothesis. The session is instead a
useful long-duration embedded MoltenVK 1.0.18 baseline with a preserved settings
and pipeline-cache end state.

Gameplay remained usable for roughly 2 hours 27 minutes. The launcher report
corresponds to a real ESO shutdown crash in audio teardown, not to the earlier
MoltenVK startup fault.

This independently grounds the previously observed
[exit-crash symptom](../TROUBLESHOOTING.md#exit-crash-and-settings-not-saving)
in an audio teardown stack, but does not establish the deeper cause of the
illegal instruction.

## Interpretation

Confirmed observations are limited to runtime identity, process duration,
shutdown crash classification, end-state hashes, and the user's qualitative
FPS report.

The option-insensitive 40-FPS state and spontaneous recovery are consistent
with a changing CPU/submission workload, object or player population, streaming
completion, or pipeline compilation/cache warm-up. They are not proof of any
one cause. In particular, this session does not demonstrate a MoltenVK 1.4.1
performance gain.

The current settings and warm cache form a checkpoint for repeat tests, but the
cache must be preserved before any cold-cache comparison and must never be
deleted as part of an experiment.

## Rollback

No game-bundle rollback was required. `scripts/status.sh` after the session
reported the original loader active, the bridge inactive, and no enable marker.

## Follow-up

Use this checkpoint for a controlled embedded-runtime repeat with a fixed route
and paired Metal HUD captures at the 40-FPS and recovered states. Record GPU
time, frame interval, app and Metal memory, thermal state, zone, camera, player
density estimate, and cache hash. Keep this performance track separate from the
next MoltenVK compatibility experiment, which still must resolve the HDR
extension-negotiation mismatch before another installation.
