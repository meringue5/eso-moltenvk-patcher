# Project status

Last updated: 2026-09-30

## 2026-09-30 ESO update

Steam installed ESO 12.1.5, databuild `3303624`, SHA-256
`027d5a6d0822ffddae308c62f5cd425b425e6188d5606d8c430988761e9e0a26`.
Public 0.2.0 correctly refuses it: its compatibility auditor pins absolute
addresses. The update relinked ESO and moved the unchanged embedded MoltenVK
object by +`0x1de7a0`. It also shipped a new original Bink generation.
Experiment [0054](experiments/0054-eso-12.1.5-relinked-update-rebase.md)
proved all 17 patch signatures, 40 external references, and GIPA/GDPA query
shapes unchanged. The selected target is now `targets-eso-2026-09-30.json`.

Local candidate `0.2.1-rc.1` skipped itself at launch because its target
lacked the inactive pacing profile. `0.2.1-rc.2` is the 0.2.0 runtime rebuilt
for the corrected target (branch `release/0.2.1`). It was installed with
settings unchanged and a verified recovery backup; Status reports `READY`.
Its first natural launch restored 60 FPS, but it again began with
`active=no` and a detached mouse until an app switch, without any sleep
(Experiment 0052 amendment). Experiment [0055](experiments/0055-activation-order-observation.md)
showed failed launches never receive AppKit activation; ESO follows AppKit
when it does. `0.2.1-rc.4`, installed with user approval, adds a one-shot
activation fallback and frontmost-app category for Experiment
[0056](experiments/0056-activation-fallback.md). It is not yet a public
release. The next gate is one natural user launch that meets the pass criteria
in Experiment 0054. This replaced the 0.2.1-logger candidate, so the `debug`
capture plan below is inactive until the logger work is rebuilt on 12.1.5.

## Current public production baseline

ESO MoltenVK Patcher 0.2.0 is the current public production release. GitHub's
latest-release endpoint selects annotated tag `v0.2.0`, which peels to release
commit `c063214ac9f0a8eaa56c11eb4c04c6dd282f1f2d`. Its one asset,
`ESO-MoltenVK-Patcher-0.2.0.zip`, has matching local and server SHA-256
`b65d608010d46836813d3a36df3bd7c44e3ada4c583cbf9e803fbe01c4c0d508`
and size 3,419,041 bytes. The server-reported download count was zero after
publication; verification did not download the asset. Public 0.1.3 remains
unchanged as the prior rollback release.

The supported exact target is macOS ESO 12.0.8, databuild `3288357`, SHA-256
`a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`,
on Apple Silicon through Rosetta. The bridge loads official MoltenVK 1.4.2 and
uses `startup-compositor-neutralize-pacing-release`: Metal argument buffers are
disabled, ESO's exact inactive 100-ms sleep branch is bypassed, generation-2
placeholder draws 71 through 149 are suppressed, normal forwarding latches at
ordinal 150, and startup bookkeeping finishes at ordinal 180.

The durable runtime and recovery evidence inherited from 0.1.3 is owned by
Experiments
[0044](experiments/0044-compositor-neutralize-pacing-bypass.md),
[0045](experiments/0045-measurement-stripped-release-profile.md),
[0046](experiments/0046-original-loader-generation-aware-recovery.md), and
[0047](experiments/0047-stable-0.1.3-release.md).

## Current 0.2.0 production package

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

The final ZIP has SHA-256
`b65d608010d46836813d3a36df3bd7c44e3ada4c583cbf9e803fbe01c4c0d508`.
Its bridge SHA-256 is
`954e8ff6cd3aceb3bfd5f874140f655aac1672394f30557eaa3bff57896683ce`.
The user-tested RC6 runtime and final installed 0.2.0 have those exact bridge
bytes; intervening RCs changed only installer, Status, tests, and documentation.

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

The exact-target and shared idle gates passed before installing the candidate
and promoting the same payload to final 0.2.0 with `--skip-settings`. Recovery
and runtime identities verify. `UserSettings.txt` remained byte-for-byte
unchanged at SHA-256
`0ae3c133862e0313e7622880effdafc7cf621074e5da6678078f881421bed178`
before the user launch. Ordinary gameplay advanced the pipeline cache to
`4f3baa1e13bc25c158f7cd3d274ebae138165d3ba9c1ff5380cee29efa076f60`;
the final package-state promotion preserved that new cache byte-for-byte.

The final bridge's production run `20260827T142452.659250000Z-pid71549`
started `active=yes`, matched the full MoltenVK configuration, activated all 17
redirects, latched after 79 suppressed draws at ordinal 150, finished at
ordinal 180, and emitted no bridge error or per-draw info row. The user reported
normal focus, no pink, and normal gameplay. Experiment
[0050](experiments/0050-architecture-backed-diagnostics-release.md) owns the
complete 0.2.0 requirements and evidence.

## Release verification

One ordinary user-controlled Steam/ZeniMax-path launch of the exact installed
bridge passed without a forced launch loop, cache replacement, settings change,
or launcher workaround. It verified:

- normal initial mouse capture;
- no visible pink placeholder;
- normal perceived FPS without material new stutter during a short ordinary
  play interval;
- a new run containing the exact runtime configuration, 17 redirects, inactive
  pacing bypass, 79 suppressed draws, ordinal-150 forwarding, ordinal-180
  finish, and no bridge error/fatal/skip record;
- no individual suppression rows at default info level; and
- the production log tightened to 0600, with rotation remaining bounded.

The user supplied the visual, focus, and performance observation because the
log cannot measure those properties. ESO, Steam, and the launcher were not
started by the agent.

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

The immediate P0 reliability issue is Experiment
[0052](experiments/0052-post-sleep-activation-divergence.md). After installing
the verified public 0.2.0 package, the user again observed detached mouse focus
and approximately 40 FPS when launching after hibernation. Exact production run
`20260830T090357.701770000Z-pid36636` loaded the expected runtime and completed
all structural startup invariants, but began `active=no` and did not record a
later active transition. The public pacing bypass was active, so this is not a
return of the former near-10-FPS 100-ms sleep path.

Public Status currently classifies that run as structurally passing; its own
contract says it cannot measure focus or FPS. Treat this as a production
reliability regression rather than acceptance evidence. The current next gate
is read-only event-order analysis and better support classification around
natural sleep/wake boundaries. Do not require Cmd-Tab recovery, force the
active byte, synthesize AppKit activation, mutate caches/settings, or resume
manual swapchain A/B runs.

Local candidate `0.2.1-logger-rc.1` was transactionally installed at 18:24 KST
on 2026-08-30, after the exact-target and idle gates passed. Its bridge SHA-256
is `e55e63d661ea4bfcf44327d14b01cb8e62d686fa4041af22032be2d06edbad99`.
The verified recovery backup remains in place, the user-customized settings
were preserved, and all pipeline-cache identities remained valid. Its
separately stored logging preference is now `debug`, mode 0600, for the next
normal ESO launch. This is a local investigation candidate, not a public
release or an acceptance result: the next required evidence is one
user-controlled natural launch and its focus/FPS observation. Experiment
[0053](experiments/0053-operational-log-level-control.md) owns the operational
and privacy rules; return the preference to `info` after the bounded capture.
