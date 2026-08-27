# Experiment 0050: architecture-backed 0.2.0 diagnostics release

- Date: 2026-08-27
- Outcome: **in progress**
- Rollback: **public 0.1.3 package and verified pristine loader remain available**

## Question

Can the host-loop and startup-compositor structure established by Experiments
0041-0049 be turned into a user-visible, privacy-preserving 0.2.0 maintenance
package without changing the validated runtime behavior?

## Product boundary

Version 0.2.0 is an architecture-backed operations release. It keeps the exact
`startup-compositor-neutralize-pacing-release` runtime profile, official
MoltenVK 1.4.2, disabled Metal argument buffers, generation-aware recovery,
and the user's accepted Balanced M4 settings checkpoint.

It does not publish unmeasured `Quality 60` or `Efficiency 60` profiles. Those
names require controlled frame-time, power, thermal, and rendering evidence.
The profile metadata added here is deliberately versioned so later validated
profiles can be added without treating unsupported guesses as product choices.

## Requirements

1. **Runtime invariance.** The 17 redirects, effective MoltenVK configuration,
   inactive 100-ms pacing bypass, 79-draw compositor window, ordinal-150
   forwarding latch, and ordinal-180 finish gate remain unchanged. Metal
   argument buffers stay disabled.
2. **Balanced profile identity.** The package records the selected settings as
   `balanced-m4-1920x1200-v1`, validates its 48 unique allowlisted keys, records
   that identity in installation state, and preserves the existing conflict-
   safe uninstall behavior.
3. **Public Status.** `Status.command` is visible beside Install and Uninstall.
   It reports ESO compatibility, bridge/recovery/settings health, installed
   release, production log location, and a bounded summary of the latest run.
   It never launches or mutates ESO, Steam, the launcher, settings, or caches.
4. **Private diagnostics export.** `Diagnostics.command` creates a local ZIP
   containing checksums, supported client identity, installer state summary,
   and a filtered latest-run log. It excludes account data, raw settings,
   caches, proprietary files, unrelated system logs, home-directory paths, and
   raw pointer-bearing trace lines.
5. **Bounded production logging.** Default info logging retains the begin,
   latch, finish, mode, pacing, configuration, and activation records required
   for Status, while demoting each of the 79 per-draw suppression records to
   debug. The bridge rotates `bridge.log` to one `bridge.log.1` at 1 MiB and
   opens logs with owner-only permissions. Logging failure remains fail-open
   for gameplay and never weakens bridge safety checks.
6. **Package integrity.** The unsigned ZIP exposes Install, Uninstall, Status,
   Diagnostics, and README; checksums cover every visible command and hidden
   payload. LF, archive-metadata, executable-bit, clean extraction, and checksum
   verification gates pass.

## Evidence gates

- `scripts/check-update.sh` and source `scripts/status.sh` recognize exact ESO
  12.0.8/databuild 3288357 before the build.
- A fresh source build passes Bink re-export, self-patch, log-policy, host-loop,
  MoltenVK configuration, compatibility, lifecycle, and Metal-backed probes.
- Log rotation has dedicated small-file, threshold, replacement, permissions,
  and error-path probe coverage.
- The release transaction fixture covers uninstalled, installed, stale-update,
  settings-applied, settings-customized, same-generation upgrade, interrupted
  install, removal, external-original preservation, generation rotation,
  latest-run classification, and privacy-filtered diagnostics export.
- The exact 0.2.0 ZIP passes clean extraction and checksum verification.
- After all non-game gates pass, one cache-preserving installation of the exact
  candidate and one user-controlled normal-launch smoke test must confirm
  initial mouse capture, no pink, normal FPS, exact 79/150/180 startup control,
  argument buffers off, and no bridge error. The agent must not launch ESO.

## Result

Implementation and agent-only verification are complete through distribution
candidate `0.2.0-rc.8`. The installed runtime candidate is RC6; RC7 and RC8
change only installer, Status, tests, and documentation, and package the exact
same bridge bytes:

- fresh source build passed Bink re-export, Rosetta self-patch, inactive pacing,
  logging, lifecycle, render-graph, reset-resource, compatibility, and all
  MoltenVK configuration probes;
- `python3 -m unittest discover -s tools -p 'test_*.py'` passed 138 tests;
- the release transaction fixture passed package-integrity refusal, Status and
  Diagnostics classification/privacy, versioned settings state, interruption
  recovery, same-generation upgrade/remove, update repair, external-original
  preservation, and original-generation rotation;
- the clean archive test passed visible-file layout, executable bits, LF,
  package checksums, privacy, and ZIP metadata hygiene;
- the RC8 ZIP SHA-256 is
  `130e9c3ae4c341eab1e4017b47ac3cc5103076ecf03b319d2144b8f215524eb9`;
- the candidate bridge SHA-256 is
  `954e8ff6cd3aceb3bfd5f874140f655aac1672394f30557eaa3bff57896683ce`;
- an actual Status run against the preceding installation classified it as a
  verified same-generation upgrade and classified the last known launch PASS;
- an actual Diagnostics ZIP was owner-only (`0600`) and contained only report,
  checksums, and filtered latest-run evidence. Searches found no home path, raw
  settings name, loaded-runtime path, branch offset, pipeline identity, or
  descriptor signature; and
- after exact-target and idle gates passed, RC6 was installed as a binary-only
  same-generation upgrade with `--skip-settings`. Status reports READY,
  recovery and runtime profile are verified, and the settings and active
  pipeline-cache SHA-256 values remain byte-for-byte unchanged at
  `0ae3c133862e0313e7622880effdafc7cf621074e5da6678078f881421bed178`
  and `c9d996a9c2207e57f3e8960e80a1023a44f01ee10edf78a64e2d782496324f88`.
- the fixture and actual RC6-to-RC8 Status check cover a same-payload package
  transition: it is reported as UPGRADE AVAILABLE, and Install re-attests the
  executable and records the newer package version instead of incorrectly
  returning “already installed.”

The exact installed bridge passed the user gate. The user reported normal play
with no problem, which covers initial focus, pink, FPS, and material stutter at
the requested observational level. New run
`20260827T142452.659250000Z-pid71549` began `active=yes`, activated all 17
redirects, matched the complete argument-buffers-off MoltenVK configuration,
latched after 79 suppressed draws at ordinal 150, and finished at ordinal 180.
It contained zero bridge error/fatal/skip rows and zero individual suppression
rows at info level. The inherited log was tightened from 0644 to 0600.

`UserSettings.txt` remained unchanged. Ordinary play legitimately advanced the
pipeline cache to SHA-256
`4f3baa1e13bc25c158f7cd3d274ebae138165d3ba9c1ff5380cee29efa076f60`;
the subsequent RC-to-final package-state promotion left both files byte-for-
byte unchanged. Final `0.2.0` Status reports READY, installed release 0.2.0,
verified recovery and runtime profile, and the new run PASS.

The final ZIP SHA-256 is
`b65d608010d46836813d3a36df3bd7c44e3ada4c583cbf9e803fbe01c4c0d508`.
Its bridge is byte-identical to the user-tested runtime. Public tag, release,
and server-asset verification remain the only pending evidence.
