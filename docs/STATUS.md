# Project status

Last updated: 2026-08-02

## Current production baseline

ESO MoltenVK Patcher 0.1.0 is the current production release. It targets the
exact macOS ESO 12.0.7 client, databuild `3281538`, on Apple Silicon through
Rosetta and loads official MoltenVK 1.4.2.

The production profile combines:

- HDR extension and surface-format compatibility for ESO's legacy Vulkan path;
- disabled Metal argument buffers;
- asynchronous queue submission and concurrent pipeline compilation;
- the validated `performance-aggressive` resource-check setting; and
- the bounded startup compositor neutralizer from Experiment 0031.

On the tested M4 MacBook Air, the validated 2048 x 1280 medium-to-high profile
held the user-observed 60 FPS VSync ceiling during roughly 93 minutes of
ordinary play. Six live graphics-device reset sequences completed without the
previous persistent solid-color result. Two controlled startups and the public
release-package run neutralized exactly 79 startup placeholder draws and
forwarded the normal scene at ordinal 150.

## Distribution status

The public distribution is the prebuilt GitHub Release ZIP. Players do not need
Python, Xcode, or a source checkout. The package provides interactive Install
and Uninstall commands, exact Steam/ZeniMax client discovery, custom-path
fallback, verified backup and recovery state, explicit settings-template
choice, and transaction recovery after an interrupted install.

Version 0.1.0 was promoted after the user installed `0.1.0-rc.1`, launched ESO
through the normal Steam authentication path, and completed ordinary gameplay
without a problem. The release-package run
`20260802T084433.627791000Z-pid88181` records the exact supported client,
official MoltenVK 1.4.2, all 17 redirects, the bounded 79-draw suppression, and
the ordinal-150 forward latch. See
[Experiment 0032](experiments/0032-release-candidate-end-to-end.md).

The signed and notarized app/DMG remains optional pending Apple Developer ID
membership. The unsigned ZIP documents Gatekeeper's Open Anyway flow.

## Current installed state

The user's validated RC remains installed on the exact current target. The
bridge, enable marker, and official 1.4.2 runtime are current. The active 1.4.2
pipeline cache, pre-bridge backup, and historical 1.4.1 cache backup all pass
their recorded identity checks.

Historical runtime and cache backups are preservation data, not supported
runtime choices. Do not delete them automatically. Source maintenance retains
only the logic required to recognize and restore those backups safely.

## Safety boundary

- Unknown ESO executable hashes, UUIDs, layouts, or patch bytes fail closed.
- Install requires a verified restore path before mutation.
- ESO, the ZeniMax launcher, active Steam ESO updates, file holders, and
  indeterminate bundle-use checks block mutation; idle Steam alone does not.
- Normal Steam or ZeniMax launcher authentication remains unchanged.
- The exact production compositor identity fails open to ESO rendering if its
  bounded repair conditions do not match.
- User settings and caches are preserved unless the player explicitly chooses
  the allowlisted settings merge.

## Next gate

The release is complete. Routine work is maintenance-driven:

1. run `scripts/check-update.sh` after an ESO update;
2. reject an unknown build until the full target profile is re-established;
3. keep 0.1.0 restore compatibility while validating any successor release;
4. expand hardware or direct-launcher claims only with matching evidence; and
5. consider a signed/notarized distribution only when its cost is justified.

Detailed historical results remain in [Findings](FINDINGS.md), the
[experiment index](experiments/README.md), and [research](research/README.md).
