# Project status

Last updated: 2026-08-27

## Current public production baseline

ESO MoltenVK Patcher 0.1.3 remains the public production release until the
0.2.0 user-launch gate and publication audit pass. The latest-release endpoint
currently selects annotated tag `v0.1.3` and its one immutable-by-policy asset,
`ESO-MoltenVK-Patcher-0.1.3.zip`, has server-reported SHA-256
`26ca4273aae669231dcc3a04e998d59b74038361e97da0b5f746434c1d02a4d7`.
The point-in-time download count remains zero as of the 0.2.0 preflight check.

The supported exact target is macOS ESO 12.0.8, databuild `3288357`, SHA-256
`a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`,
on Apple Silicon through Rosetta. The bridge loads official MoltenVK 1.4.2 and
uses `startup-compositor-neutralize-pacing-release`: Metal argument buffers are
disabled, ESO's exact inactive 100-ms sleep branch is bypassed, generation-2
placeholder draws 71 through 149 are suppressed, normal forwarding latches at
ordinal 150, and startup bookkeeping finishes at ordinal 180.

The durable runtime and recovery evidence for 0.1.3 is owned by Experiments
[0044](experiments/0044-compositor-neutralize-pacing-bypass.md),
[0045](experiments/0045-measurement-stripped-release-profile.md),
[0046](experiments/0046-original-loader-generation-aware-recovery.md), and
[0047](experiments/0047-stable-0.1.3-release.md).

## 0.2.0 release candidate

Version 0.2.0 is an architecture-backed operations release. It deliberately
retains the exact 0.1.3 runtime control instead of introducing another startup
or performance variable. It adds:

- visible read-only `Status.command` with exact client, bridge, recovery,
  settings-profile, package-version, and latest-run classification;
- visible `Diagnostics.command` producing a 0600 privacy-filtered support ZIP;
- versioned settings profile `balanced-m4-1920x1200-v1`, containing exactly 48
  selectively merged keys and preserving customized settings on removal;
- checksum verification of every visible command and hidden payload before any
  action;
- 1 MiB production-log rotation with one retained generation, owner-only file
  permissions, and the 79 repetitive per-draw records removed from info logs;
  and
- correct same-payload RC-to-final re-attestation and package-version status.

Distribution candidate `0.2.0-rc.8` has ZIP SHA-256
`130e9c3ae4c341eab1e4017b47ac3cc5103076ecf03b319d2144b8f215524eb9`.
Its bridge SHA-256 is
`954e8ff6cd3aceb3bfd5f874140f655aac1672394f30557eaa3bff57896683ce`.
The installed RC6 runtime has that exact bridge SHA; RC7 and RC8 changed only
installer, Status, tests, and documentation.

Agent-only gates currently pass:

- exact update/profile check for ESO 12.0.8/databuild 3288357;
- fresh build with Bink re-export, Rosetta self-patch, inactive pacing, log
  policy/file, compatibility, lifecycle, reset-resource, render-graph, and all
  MoltenVK configuration probes;
- 138 Python tests;
- the full installer fixture, including corruption refusal, Status and
  Diagnostics privacy, settings apply/customize/remove, interruption recovery,
  same-generation bridge replacement, same-payload package promotion, stale
  update refusal, external-original preservation, and generation rotation; and
- clean archive layout, executable bits, LF, checksums, prohibited-file search,
  and ZIP metadata hygiene.

The exact-target and shared idle gates passed before installing RC6 with
`--skip-settings`. Recovery and runtime identities verify. `UserSettings.txt`
and the active pipeline cache remained byte-for-byte unchanged at SHA-256
`0ae3c133862e0313e7622880effdafc7cf621074e5da6678078f881421bed178`
and `c9d996a9c2207e57f3e8960e80a1023a44f01ee10edf78a64e2d782496324f88`.

The latest production log still belongs to the preceding control run,
`20260827T123612.412745000Z-pid374`; its PASS classification proves the common
runtime invariants but is not evidence that the newly installed candidate has
launched. Experiment [0050](experiments/0050-architecture-backed-diagnostics-release.md)
owns the complete 0.2.0 requirements and evidence.

## Active release gate

One ordinary user-controlled Steam/ZeniMax-path launch of the exact installed
bridge remains mandatory. No forced launch loop, cache replacement, settings
change, or launcher workaround is requested. The gate requires:

- normal initial mouse capture;
- no visible pink placeholder;
- normal perceived FPS without material new stutter during a short ordinary
  play interval;
- a new run containing the exact runtime configuration, 17 redirects, inactive
  pacing bypass, 79 suppressed draws, ordinal-150 forwarding, ordinal-180
  finish, and no bridge error/fatal/skip record;
- no individual suppression rows at default info level; and
- the production log tightened to 0600, with rotation remaining bounded.

User observation is required because the log cannot measure FPS, image color,
or effective mouse capture. ESO, Steam, and the launcher are never started by
the agent.

## Known limits

- The runtime result and Balanced profile are target-specific M4 evidence, not
  a universal 60-FPS guarantee for every Mac or scene.
- The approximately 93-minute 60-FPS VSync observation belongs to the earlier
  2048 x 1280 gameplay checkpoint. The selected 1920 x 1200 Balanced profile
  passed approximately 54 minutes of ordinary play but lacks continuous frame-
  time, power, and thermal capture.
- Earlier pink/low-FPS recurrence, failed readiness canary, no-neutralizer
  control, and rejected Metal argument-buffer candidate remain preserved in
  Experiments 0035-0049 and [Findings](FINDINGS.md); they are not active product
  modes.
- Signed and notarized app/DMG distribution remains optional future work. The
  public artifact is the unsigned ZIP with documented Gatekeeper handling.

## Safety boundary

- An exact target is accepted directly. A different executable must reproduce
  the complete packaged structural fingerprint; any changed embedded MoltenVK,
  patch byte, old-runtime reference boundary, or proc-query route fails closed.
- Install and Uninstall require a verified same-generation restore record.
  A bridge retained across an executable update requires launcher Repair; an
  externally restored original is preserved byte-for-byte.
- ESO, the ZeniMax launcher, active Steam ESO updates, bundle file holders, and
  indeterminate idle checks block mutation. Idle Steam alone is not a blocker.
- Settings and caches remain untouched unless the player explicitly selects
  the allowlisted settings merge. Historical backups are preservation data and
  are never automatically deleted.

## Next gate

After the user observation and new log both pass, rebuild the same source as
`0.2.0`, verify that its bridge SHA matches the tested runtime, update the
production baseline and experiment result, commit and push `main`, create and
push annotated tag `v0.2.0`, publish a new GitHub Release ZIP without modifying
0.1.3, and verify the latest endpoint, tag target, server asset digest, asset
layout, and download count without downloading the asset.
