# Project status

Last updated: 2026-08-27

## Current production baseline

ESO MoltenVK Patcher 0.1.2 is the current production maintenance release. Its
selected exact target is macOS ESO 12.0.8, databuild `3288357`, on Apple
Silicon through Rosetta, and it loads official MoltenVK 1.4.2. The extended
performance baseline remains the 12.0.7 gameplay checkpoint; 12.0.8 passed a
bounded user-controlled Steam-path startup and gameplay validation.

The production profile combines:

- HDR extension and surface-format compatibility for ESO's legacy Vulkan path;
- disabled Metal argument buffers;
- asynchronous queue submission and non-maximized pipeline compilation;
- the validated `performance-aggressive` resource-check setting; and
- the performance-first Experiment 0038 path with compositor neutralization
  and its supporting startup audits disabled.

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

Version 0.1.2 retains 0.1.1's ESO 12.0.8 support and packaged compatibility
auditor, then promotes the Experiment 0038 performance-first startup profile.
An updated executable is accepted only when its embedded
MoltenVK archive is unchanged and its exact patch bytes, complete old-runtime
reference boundary, and proc-query shape match the compiled profile. Install
then records the audited executable hash, and the runtime rechecks that
attestation before redirecting. Launcher-restored-original and
bridge-retained update states are both covered by the disposable release
transaction fixture.

The public `v0.1.2` release is live. Its freshly downloaded ZIP and GitHub's
server-reported asset digest both match SHA-256
`7b587caa68bf729ec4ea75888223c72908672fb70c853a382d7215407d21e830`.
The packaged bridge is byte-identical to the user-validated Experiment 0038
installation. See Experiment
[0039](experiments/0039-performance-first-0.1.2-release.md).

Release packaging and digests remain verified, but 0.1.2 has an open
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

The user has the Experiment 0044 source candidate installed on the exact
12.0.8 target. Its marker selects
`startup-compositor-neutralize-pacing-bypass`; its bridge, retagged original
Bink, and official MoltenVK 1.4.2 payloads exactly match the committed build
identities. The pristine loader matches the selected target manifest. The
active 1.4.2 pipeline cache, pre-bridge backup, historical 1.4.1 cache backup,
and `UserSettings.txt` retain their identities across the cache-preserving
restore/install cycle, and all three cache headers pass.

The candidate retains 0041's exact inactive 100-ms outer-loop sleep bypass,
the current non-maximized MoltenVK compilation policy, and bounded first-64
graphics-pipeline timing. It combines those with Experiment 0031's proven
fixed-window neutralizer: only exact final-compositor draws at generation-2
ordinals 71 through 149 are replaced by opaque black, and ordinal 150 onward
is forwarded. Pixel readback and the 0043 image sampler are disabled. AppKit
callbacks, focus propagation, settings, caches, and the launcher path remain
unchanged. Source, non-game, restore, installation, payload-identity, cache-
identity, and settings-preservation gates pass. This installed state is a
diagnostic release candidate, not a public release claim. See
[ESO host runtime structure](ESO-HOST-RUNTIME.md) and Experiment
[0044](experiments/0044-compositor-neutralize-pacing-bypass.md).

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

The 0.1.2 release-reliability incident is open, but its leading mechanism is
now localized outside MoltenVK. The exact ESO main loop sleeps for 100,000
microseconds per iteration whenever its internal AppKit active-state byte is
false, matching the observed approximately 10-FPS mode. Experiment
[0041](experiments/0041-inactive-pacing-bypass.md) implements the
single-variable, exact-target runtime bypass of only that inactive-loop sleep,
with bounded logging of the internal false state. Its build, dedicated x86_64
patch probe, 134 Python tests, release transaction regression, static checks,
and both Metal-backed Vulkan probes pass. After the user closed the launcher,
a cache-preserving restore/install cycle passed the shared bundle-idle gate.
The installed bridge, renamed original, and MoltenVK hashes exactly match the
built candidate; the marker selects `startup-inactive-pacing-bypass`; the
restore source remains available; and every preserved cache identity passes.

The initial user gate now contains two normal-FPS starts with pink and
`active=yes`. Continue ordinary use without forced retries. The decisive next
evidence is the first natural `active=no` or low-FPS start, correlated with the
bounded state log. Preserve the current MoltenVK runtime, compilation policy,
settings, caches, focus-event propagation, and launcher path. Device/swapchain/
present tracing is no longer the first low-FPS diagnostic boundary unless the
focused bypass fails to remove the low-FPS state.

Visible pink alone remains cosmetic; do not reintroduce its neutralizer or
claim launcher/cache causality while the activation event-order trigger is
unresolved.

At the user's direction, the optional pink-repair audit keeps 0041's inactive-
sleep bypass, non-maximized compilation, and bounded pipeline timing. It
forwards all ESO draws unchanged and only samples the bound `Sampler0` scene
and `Sampler1` GUI images during the bounded startup window. Experiment 0042
proved the forward-only path can retain normal FPS with visible pink but lost
its evidence to log filtering. Experiment 0043 corrected the bounded log
visibility and closed the source boundary: the GUI-classified second image
directly contains the full-screen magenta and changes contents in place.

Experiment [0044](experiments/0044-compositor-neutralize-pacing-bypass.md)
combines the already validated Experiment 0031 fixed-window neutralizer with
the independent inactive pacing bypass. It disables pixel readback and adds no
new rendering mutation. Its fresh build, focused mode/analyzer/log probes, 136
Python tests, release transaction regression, syntax, and diff checks pass.
Official and embedded Metal-backed non-game probes also pass. The exact-target
cache-preserving installation passed the shared bundle-idle, restore, payload,
cache, and settings-preservation gates. One ordinary user-controlled launch
must now report both pink visibility and FPS state.

Detailed historical results remain in [Findings](FINDINGS.md), the
[experiment index](experiments/README.md), and [research](research/README.md).
