# Project status

Last updated: 2026-08-27

## Current production baseline

ESO MoltenVK Patcher 0.1.3 is the current production maintenance release. Its
selected exact target is macOS ESO 12.0.8, databuild `3288357`, on Apple
Silicon through Rosetta, and it loads official MoltenVK 1.4.2. The extended
performance baseline remains the 12.0.7 gameplay checkpoint; 12.0.8 passed a
bounded user-controlled Steam-path startup and gameplay validation.

The production profile combines:

- HDR extension and surface-format compatibility for ESO's legacy Vulkan path;
- disabled Metal argument buffers;
- asynchronous queue submission and non-maximized pipeline compilation;
- the validated `performance-aggressive` resource-check setting; and
- the exact inactive 100-ms host-pacing bypass;
- the bounded 79-draw compositor repair with ordinal-150 fail-open forwarding;
  and
- measurement-stripped post-window direct forwarding with pipeline timing
  disabled.

Performance successor research is active while leaving public 0.1.3 unchanged.
Experiment 0048 measured the residual cached-wrapper tax at
only single-digit nanoseconds per hot call and deferred a self-retiring
trampoline. Experiment 0049 found a 14.899% CPU descriptor-encoding gain from
MoltenVK 1.4.2 Metal argument buffers and exact-pixel passes in both 24-cycle
reset configurations. Its fail-closed experimental mode remains installed
after two ESO runs without a reported recurrence of historical corruption.
The second, approximately 27-minute warm run had no stutter or loaded-world
graphics reset, strengthening the cache/reset-transient interpretation of the
first run. The raised High profile still produced a user-perceived frame-rate
loss in object-dense play and did not hold the 60-FPS quality target. The
historical 1.4.1 rendering-corruption boundary and lack of a same-settings
control still block adoption pending a controlled argument-buffer A/B.

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

Version 0.1.3 retains ESO 12.0.8 support and the packaged compatibility auditor,
then promotes the Experiment 0045 measurement-stripped startup profile.
An updated executable is accepted only when its embedded
MoltenVK archive is unchanged and its exact patch bytes, complete old-runtime
reference boundary, and proc-query shape match the compiled profile. Install
then records the audited executable hash, and the runtime rechecks that
attestation before redirecting. Experiment 0046 binds that executable and the
original Bink hash as one recovery generation. A verified earlier patcher may
upgrade or uninstall directly on the same ESO and original generation; a
bridge retained across an executable update requires launcher Repair, and a
different launcher-provided original is preserved byte-for-byte.

The public `v0.1.3` release is live and selected by GitHub's latest-release
endpoint. Its single `ESO-MoltenVK-Patcher-0.1.3.zip` asset has local and
server-reported SHA-256
`26ca4273aae669231dcc3a04e998d59b74038361e97da0b5f746434c1d02a4d7`.
The annotated tag resolves to release commit `bc3a5b8`, and the point-in-time
download count was 0 immediately after publication. The public asset was not
downloaded during verification, avoiding an artificial counter increment. See
Experiment [0047](experiments/0047-stable-0.1.3-release.md).

Version 0.1.2 remains preserved as historical evidence but has an open
reliability incident: the exact installed no-neutralizer profile later
reproduced pink and low FPS. It is not a demonstrated low-FPS repair. See
Experiment [0040](experiments/0040-no-neutralizer-low-fps-recurrence.md).

The 12.0.8 user run activated all 17 redirects and reached the same 79-draw,
ordinal-150 compositor latch in three consecutive starts. The final start and
gameplay were normal. See Experiment
[0034](experiments/0034-eso-12.0.8-update-compatible-recovery.md).

The signed and notarized app/DMG remains optional pending Apple Developer ID
membership. The unsigned ZIP documents Gatekeeper's Open Anyway flow.

## Current installed state

Experiment 0049's source candidate is installed on exact ESO 12.0.8 with
marker `startup-release-argument-buffers`. It is byte-identical to the current
validated build and changes one runtime variable from public 0.1.3: Metal
argument buffers are enabled. The public 0.1.3 package remains the production
and rollback reference; the experimental candidate is not released or
supported pending its user-controlled rendering and performance gates.

First installed run `20260827T105710.534190000Z-pid87213` activated the exact
candidate without bridge errors, recovered from initial stutter to the 60-FPS
VSync ceiling, and retained intermittent stutter. At settings materially higher
than the early baseline, an object-dense area became intolerably slow before
the user changed resolution once and HBAO/SSAO several times. ESO recorded 13
graphics-device/swapchain resets, while the MoltenVK pipeline cache changed and
Metal compiler warnings continued sporadically. This is a provisional safety
pass, not a performance pass or evidence that the candidate can hold 60 FPS at
the raised quality profile.

Warm run `20260827T110923.707262000Z-pid88869` lasted approximately 27 minutes
across Khenarthi's Roost, Vvardenfell, and Stros M'Kai. It had no reported
stutter, no loaded-world reset, and no bridge error, while the pipeline cache
grew to 15,842,531 bytes. The user still perceived frame-rate loss and judged
the High profile too demanding. No continuous FPS or GPU-time sample exists,
so this is a qualitative quality-ceiling result. The next gate settles a
medium-to-high profile and performs an argument-buffers-off versus-on frame-
time A/B; another forced warm-up launch is unnecessary.

The candidate retains Experiment 0044's exact inactive
100-ms host-pacing bypass, fixed generation-2 ordinal 71-149 compositor
substitution, and ordinal-150 fail-open forwarding latch. It removes the first-
64 graphics-pipeline timing path and makes every identified lifecycle wrapper
direct-forward without table mutation after the ordinal-180 finished gate.
Pixel readback and compositor image sampling remain disabled. Both active and
old pipeline-cache files were preserved in place during the restore/install
cycle.

The 0.1.3 build, 138 Python tests, generation-aware installer transaction
fixture, official and embedded Metal probes, startup-surface probe, ten runtime-
readiness compiler canaries, internal checksums, and ZIP hygiene all pass. The
cache-preserving restore/install transaction left `UserSettings.txt`, the
active 1.4.2 cache, and the old-runtime cache backup byte-identical. The
verified public-installer backup and Uninstall path remain available. Exact run
`20260827T071558.350339000Z-pid73173` passed the final user gate with no pink
and normal FPS. It logged 79 suppressions at ordinals 71-149, one ordinal-150
forward latch, ordinal-180 completion, no pipeline timing, and no errors or
overflows. A short battery-mode mid-high observation remained smooth but is not
a sustained battery or cross-hardware benchmark. See Experiment
[0045](experiments/0045-measurement-stripped-release-profile.md).

Two consecutive ordinary user-controlled starts passed the 0044 gate with no
visible pink and normal FPS. Exact runs
`20260827T060530.866920000Z-pid39106` and
`20260827T060606.566512000Z-pid39189` each suppressed exactly 79 target draws,
forwarded at ordinal 150, finished at ordinal 180, and retained 64/64
successful non-null graphics-pipeline calls without bounded lifecycle errors.
The first run observed `active=yes`; the second observed `active=no` and
recorded `action=sleep-bypassed`, directly exercising the patched 100-ms sleep
branch. This is a two-run functional pass, not yet a long-term reliability or
public-release claim.

The first 0042 launch had normal FPS with pink, but its compositor verdict is
inconclusive because the production `info` log discarded every analyzer-
required bounded audit record as `debug` or `trace`. Experiment
[0043](experiments/0043-bounded-audit-log-visibility.md) fixes only that
evidence path. Its logging policy is covered by a dedicated probe, and its
fresh build, 135 Python tests, release transaction regression, static checks,
and official/embedded Metal-backed probes pass. A cache-preserving restore and
installation completed after the shared bundle-idle gate passed; the installed
payloads match the built candidates and the preserved user-file hashes did not
change. The replacement launch then retained normal FPS with visible pink and
produced the decisive
`COMPOSITOR-GUI-MAGENTA-IN-PLACE-CONTENT-CHANGE` verdict. The second
GUI-classified compositor image contained exact magenta at all sampled points
in ordinals 80 through 140 and ordinary colors at ordinals 150 through 180
without changing image identity. The first scene-classified image was not
magenta in either interval.

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
Source and non-game gates passed, and the candidate is now installed.

The first installed Experiment 0037 launch produced normal extended play. All
64 retained graphics-pipeline calls returned `VK_SUCCESS` with non-null output;
the slowest took 1.783 ms. ESO advanced directly to character selection and
marked the renderer complete about 2.77 seconds later, matching the normal
path rather than the roughly 13-second low-FPS path. The unchanged compositor
neutralizer still suppressed exactly 79 draws and forwarded at ordinal 150.
This is one positive result for non-maximized compilation and a direct
counterexample to neutralization alone being sufficient to cause low FPS, not
yet a repeatability claim.

That candidate later reproduced visible pink and low FPS. All 64 retained
graphics-pipeline calls succeeded with non-null outputs and a maximum duration
of 6.775 ms, but ESO issued the bulk wave about 20.8 seconds later than in the
first normal Experiment 0037 run. Renderer completion was again delayed to
about 13.86 seconds after character selection. Non-maximized compilation is
therefore falsified as a reliability repair, while the timing evidence moves
the fault upstream of actual graphics-pipeline compilation.

Experiment 0038 is the selected performance-first control. It keeps non-maximized
compilation and bounded pipeline timing but disables only compositor
neutralization and all supporting startup audits. Source and non-game gates
pass, and the exact-target cache-preserving transaction installed the control.
The first user-controlled launch retained pink but followed the normal FPS and
renderer path: call 5 began at 11.652 seconds and renderer completion followed
character selection by 2.685 seconds. Pink is expected and is not failure.

Experiment 0040 falsifies that control as a reliability repair. Exact run
`20260826T173857.097445000Z-pid322` used the 0.1.2 mode with neutralization and
all supporting audits disabled, yet the user observed pink and low FPS. All 64
retained graphics-pipeline calls succeeded; call 5 began at 32.698 seconds and
completed in 1.425 ms. ESO took 13.762 seconds from `CharacterSelect` to
`RENDERER Complete`. The shader cache stayed byte-identical to the normal 0038
run. The fault therefore remains upstream of actual graphics-pipeline
creation, and compositor neutralization is excluded as a necessary cause.

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

Experiment [0044](experiments/0044-compositor-neutralize-pacing-bypass.md) has
passed its declared user gate twice. Most importantly, the second normal-FPS
start entered `active=no` and exercised the exact inactive-sleep bypass while
the fixed-window compositor neutralizer still removed the visible magenta.
This is the first direct combined evidence for both intended repairs.

Keep the exact candidate installed for ordinary use without forced launch
loops, cache replacement, settings changes, or launcher workarounds. The next
gate is natural-use soak. If pink or low FPS recurs, preserve the exact run and
classify its pacing state, neutralizer window, pipeline timing, and ESO renderer
timing before changing the candidate. If repeated ordinary starts remain clean,
promote this exact committed configuration through a new immutable release
rather than replacing the existing 0.1.2 asset.

The event-order condition that sometimes leaves ESO internally inactive is
still unresolved, and two starts do not close long-term cold-start reliability.
A bounded forward-only device/swapchain/queue/present trace becomes relevant
only if low FPS returns despite a recorded `action=sleep-bypassed`.

Detailed historical results remain in [Findings](FINDINGS.md), the
[experiment index](experiments/README.md), and [research](research/README.md).
