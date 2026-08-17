# Project status

Last updated: 2026-08-17

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

The user has the Experiment 0037 source candidate, derived from public 0.1.1,
installed on the exact 12.0.8 target. Its bridge, enable marker, and official
1.4.2 runtime are current. The active 1.4.2 pipeline cache, pre-bridge backup,
and historical 1.4.1 cache backup all pass their recorded identity checks. The
installed bridge is byte-identical to committed source build `30b499a`, uses
MoltenVK's default non-maximized compilation policy, omits the failed readiness
canary, and records bounded graphics-pipeline call timing. Installation
followed a verified pristine restore and preserved all caches and settings.
This installed state is not a public release claim.

Historical runtime and cache backups are preservation data, not supported
runtime choices. Do not delete them automatically. Source maintenance retains
only the logic required to recognize and restore those backups safely.

## Known cold-start reliability issue

The user reports a recurring post-install pattern: a start may show the pink
splash and then run at approximately 10 FPS, while an immediate restart is
clean and smooth. Experiment 0035 captured one such back-to-back pair without a
launcher restart. Both processes loaded MoltenVK 1.4.2, activated all 17
redirects, and reached the same 79-draw/ordinal-150 compositor latch, excluding
total bridge nonactivation for the bad process.

The bad process recorded no ESO-side `MTLCompilerService` connection or Metal
compilation job during its approximately 42-second lifetime. The smooth process
started four seconds later and recorded ten connection events and six
successful compilation jobs, beginning approximately 27 seconds after launch.
This makes failure to enter the normal Metal pipeline-compilation path the
leading hypothesis. It does not yet prove whether compiler-service absence is
the cause or a consequence, and the run lacks fixed-scene FPS/GPU-time
telemetry and an intermediate cache snapshot.

Experiment 0036 produced a source readiness-gate candidate. Immediately
after `VkDevice` creation, it compiles a process-unique, cache-independent
compute pipeline before returning the device to ESO. Five non-game trials under
the exact production configuration each forced one Metal library build and one
pipeline build; all ten compiler jobs succeeded. Failure-path probes verify
that temporary objects and the device are cleaned before an error is returned.

ESO validation failed the candidate's purpose. A smooth session and the next
low-FPS session each made ten compiler-service connections and immediately
completed the canary's one library and one pipeline job without failure. The
smooth session later completed 33 additional ESO compilation jobs; the low-FPS
process completed none beyond the canary during its approximately 105-second
lifetime. Compiler-service reachability is therefore insufficient to make ESO
enter its normal graphics-pipeline path. The bridge log also omitted the
required readiness success record in both runs, leaving a separate
observability defect. The candidate remains installed as a failed checkpoint
with a verified restore path; it is not eligible for packaging or release.

A second consecutive low-FPS process reproduced the same canary-only signature:
ten compiler-service connections, two successful immediate jobs, and no later
ESO compilation. One restart therefore does not deterministically repair the
condition. The same-size active pipeline cache changed SHA-256 between the two
failed exits while `ShaderCache.cooked` remained unchanged. Repeated-exit cache
evolution is now a specific alternative to a probabilistic startup race, but
causality is unproven because the first cache generation was not preserved as
bytes before the second run.

A later four-run sequence initially appeared to sharpen the discriminator. Three consecutive
pink/low-FPS starts with the ZeniMax launcher left open each completed only the
two forced canary jobs. After the user restarted the launcher through Steam,
the next ESO process was normal and performed one additional successful
`MTLBuildOpaqueRequest` about eight seconds after start. Launcher restart is
correlated with this recovery but is not treated as causal or critical: the
user reports that other recoveries have occurred through ESO-only retries.

The subsequent completed normal session invalidated additional compiler work
as an early classifier. That process remained canary-only for approximately 9
minutes 34 seconds, well beyond the complete lifetime of the short failed
runs, while the user observed normal performance. Its three later compilation
jobs all succeeded, but were not required for initial normal rendering.
Compiler-service logs therefore show that the service is available; they do
not expose the causal startup transition.

The next post-sleep first launch reproduced pink/low FPS about 41 minutes after
a recorded full wake, weakening a simple immediate post-wake readiness race.
ESO's own interface log supplies a stronger discriminator: low starts enter
the game-data and character-data wait states and take about 13 seconds to mark
the renderer complete, while the preceding completed normal start advanced
directly to character selection and completed the renderer in about 2.7
seconds. Experiment 0037 therefore restores MoltenVK's default non-maximized
compilation policy for the production profile, removes the failed canary, and
adds bounded no-op timing around the first 64 graphics-pipeline creation calls.
Source and non-game gates pass; installation and user validation are pending.

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

Collect one ordinary user-controlled launch of the installed Experiment 0037
candidate. Require matched pipeline begin/end records and compare the
renderer-completion path with the user's pink/FPS observation. Do not package
the candidate or alter user caches until repeated evidence supports the
non-maximized compilation policy. Record launcher lifetime only as a secondary
correlation field.

Detailed historical results remain in [Findings](FINDINGS.md), the
[experiment index](experiments/README.md), and [research](research/README.md).
