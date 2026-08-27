# Project status

Last updated: 2026-08-27

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

Monitor natural launches through public Status and privacy-filtered Diagnostics
without forced repetition or cache deletion. Preserve exact run evidence before
changing the production baseline if pink, low FPS, focus loss, update recovery,
or uninstall behavior regresses. Performance and quality successors remain
separate single-variable work under [Roadmap](ROADMAP.md).
