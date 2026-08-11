# Project status

Last updated: 2026-08-11

## Current production baseline

ESO MoltenVK Patcher 0.1.1 is the current production maintenance release. Its
selected exact target is macOS ESO 12.0.8, databuild `3288357`, on Apple
Silicon through Rosetta, and it loads official MoltenVK 1.4.2. The extended
performance baseline remains the 12.0.7 gameplay checkpoint; 12.0.8 passed a
bounded user-controlled Steam-path startup and gameplay validation.

The production profile combines:

- HDR extension and surface-format compatibility for ESO's legacy Vulkan path;
- disabled Metal argument buffers;
- asynchronous queue submission and concurrent pipeline compilation;
- the validated `performance-aggressive` resource-check setting; and
- the bounded startup compositor neutralizer from Experiment 0031.

The bundled M4 settings template now selects High subsampling
(`SUB_SAMPLING "2"`). The user validated that setting in ordinary gameplay on
the same exact production build and reported no problem.

On the tested M4 MacBook Air, the validated 2048 x 1280 medium-to-high profile
held the user-observed 60 FPS VSync ceiling during roughly 93 minutes of
ordinary 12.0.7 play. Six live graphics-device reset sequences completed without the
previous persistent solid-color result. Two controlled startups and the public
release-package run neutralized exactly 79 startup placeholder draws and
forwarded the normal scene at ordinal 150.

## Distribution status

The public distribution is the prebuilt GitHub Release ZIP. Players do not need
Python, Xcode, or a source checkout. The package provides interactive Install
and Uninstall commands, exact Steam/ZeniMax client discovery, custom-path
fallback, verified backup and recovery state, explicit settings-template
choice, and transaction recovery after an interrupted install.

Version 0.1.1 adds ESO 12.0.8 and a packaged compatibility auditor for future
game updates. An updated executable is accepted only when its embedded
MoltenVK archive is unchanged and its exact patch bytes, complete old-runtime
reference boundary, and proc-query shape match the compiled profile. Install
then records the audited executable hash, and the runtime rechecks that
attestation before redirecting. Launcher-restored-original and
bridge-retained update states are both covered by the disposable release
transaction fixture.

The 12.0.8 user run activated all 17 redirects and reached the same 79-draw,
ordinal-150 compositor latch in three consecutive starts. The final start and
gameplay were normal. See Experiment
[0034](experiments/0034-eso-12.0.8-update-compatible-recovery.md).

The signed and notarized app/DMG remains optional pending Apple Developer ID
membership. The unsigned ZIP documents Gatekeeper's Open Anyway flow.

## Current installed state

The user has the runtime-identical 0.1.1 RC bridge installed on the exact
12.0.8 target. The bridge, executable-hash enable marker, and official 1.4.2
runtime are current. The active 1.4.2 pipeline cache, pre-bridge backup, and
historical 1.4.1 cache backup all pass their recorded identity checks.

Historical runtime and cache backups are preservation data, not supported
runtime choices. Do not delete them automatically. Source maintenance retains
only the logic required to recognize and restore those backups safely.

## Known cold-start reliability issue

The user reports a recurring post-install pattern: the first one or two starts
may show the pink splash and then run at approximately 10 FPS; restarting ESO,
and sometimes the launcher, produces clean normal operation. The three
12.0.8 runs all reached the same production bridge activation and compositor
latch, so total bridge activation failure is excluded. No controlled per-start
cache, shader-compilation, GPU-time, or process-lifetime capture exists yet.
Shader or pipeline-cache warm-up is therefore a hypothesis, not a finding.

## Safety boundary

- An exact selected ESO target is accepted directly. A different executable is
  accepted only by the packaged structural compatibility audit; changed
  embedded MoltenVK, patch bytes, reference boundary, or proc routes fail
  closed.
- Install requires a verified restore path before mutation.
- ESO, the ZeniMax launcher, active Steam ESO updates, file holders, and
  indeterminate bundle-use checks block mutation; idle Steam alone does not.
- Normal Steam or ZeniMax launcher authentication remains unchanged.
- The exact production compositor identity fails open to ESO rendering if its
  bounded repair conditions do not match.
- User settings and caches are preserved unless the player explicitly chooses
  the allowlisted settings merge.

## Next gate

Run one bounded cold-start comparison before changing the production profile:

1. snapshot pipeline-cache identity, size, and mtime before and after every
   start;
2. record ESO and launcher process lifetimes and fixed startup milestones;
3. capture FPS, GPU time, frame interval, memory, and thermal state in the same
   scene for the first bad and first clean starts;
4. inspect MoltenVK pipeline/shader timing only after the low-overhead evidence
   identifies a discriminating interval; and
5. attempt prewarming only if the evidence demonstrates a safe cache or shader
   dependency that can be prepared without launching ESO or bypassing its
   normal authentication path.

Detailed historical results remain in [Findings](FINDINGS.md), the
[experiment index](experiments/README.md), and [research](research/README.md).
