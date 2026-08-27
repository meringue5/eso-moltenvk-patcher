# Experiment 0049: Metal argument-buffer performance candidate

- Date: 2026-08-27
- Outcome: **rejected after three consecutive startup mouse-focus failures;
  cache-preserving argument-buffers-off control installed**
- Rollback: **verified pristine loader and public 0.1.3 reference remain available**

## Question

Does official MoltenVK 1.4.2 make Metal argument buffers a worthwhile and
sufficiently controlled successor experiment despite the rendering corruption
observed with MoltenVK 1.4.1 in Experiment 0005?

## Hypothesis

MoltenVK documents Metal argument buffers as a generally faster descriptor
path, and 1.4.2 includes argument-buffer alignment and correctness fixes. The
candidate should measurably reduce descriptor encoding time and pass the
existing long-lived descriptor/reset composite before it is considered for one
user-controlled ESO A/B. Any pixel mismatch, reset failure, startup regression,
black-layer flicker, or solid-color output rejects it.

## Target and single variable

- ESO: exact 12.0.8/databuild `3288357` target.
- Runtime: official MoltenVK 1.4.2.
- Reference: exact 0.1.3 `startup-compositor-neutralize-pacing-release`
  behavior with `MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0`.
- Candidate mode: `startup-release-argument-buffers`, identical except
  `MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=1`.
- Caches and ESO graphics settings remain unchanged.

## Non-game performance evidence

The M4 x86_64/Rosetta descriptor probe recorded 20,000 alternating-resource
draws per sample, seven samples per process, and three balanced processes per
configuration. Live-resource checking was disabled and synchronous submission
was used only to keep CPU command encoding inside the measured interval.

| Configuration | Aggregate median submit | Median per draw | Process medians |
|---|---:|---:|---|
| Argument buffers off | 3,529,500 ns | 176.475 ns | 178.121, 176.802, 167.458 ns |
| Argument buffers on | 3,003,625 ns | 150.181 ns | 149.233, 150.281, 150.829 ns |

The measured descriptor-encoding reduction is **14.899%**. Every sample
produced the expected pixel. This is not a claim of a 14.899% ESO FPS gain.

## Non-game correctness evidence

Both configurations passed all 24 reset-composite cycles with:

- alternating 2048 x 1280 and 1920 x 1200 source resources;
- one full-lifetime descriptor set updated across cycles;
- alternating command-buffer and command-pool reset;
- asynchronous queue submission and non-maximized compilation; and
- exact expected Metal pixel output in every cycle.

## Source candidate

The source adds one fail-closed experimental marker mode. It retains the 0.1.3
inactive-pacing bypass, compositor repair window, ordinal-150 forward latch,
ordinal-180 finish, stripped timing, HDR filters, asynchronous submit, MTLHeap,
command pooling, and disabled live-resource checks. Configuration verification
requires argument buffers to report enabled before any ESO patch site is
written.

The complete source build passed Bink re-export, Rosetta self-patching, all
existing C/Objective-C smoke probes, the new exact configuration probe, 138
Python tests, Python compilation, shell syntax, whitespace checks, and the
release-installer transaction regression.

## Installation

The shared bundle-idle gate reported that Steam was open but had no ESO file or
update activity. A cache-preserving restore then verified the exact 12.0.8
target and pristine loader before replacing the bridge. The experimental mode
was installed with both pipeline caches left in place.

Post-install verification reports:

```text
mode:              startup-release-argument-buffers
bridge SHA-256:    fe0295de932ab34b5dc85a7fec40edecbd85ee8d54f0080edc15be072defc805
MoltenVK SHA-256:  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
active cache:      f271894e906a4177fc90d50dc645ee7efe114e64ceb20b7573a78a7ed3554b48
old cache:         72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
```

The installed bridge and MoltenVK are byte-identical to the validated build.
No agent launched Steam, the launcher, or ESO.

## User rendering gate

One normal Steam/launcher launch is sufficient for the first safety gate:

1. Observe character selection for rendering corruption.
2. Enter the world only if character selection is correct.
3. Keep current graphics settings unchanged and play for at most five minutes.
4. Stop immediately for black/shadow flicker, solid-color output, a crash,
   severe stutter, low FPS, or another obvious regression.
5. Report rendering correctness and whether FPS feels normal; controlled
   performance measurement is a later gate, not part of this first launch.

## First installed ESO run

The user launched through the normal Steam/launcher path and played run
`20260827T105710.534190000Z-pid87213` for approximately seven minutes. The
exact candidate was active with Metal argument buffers enabled, 79 compositor
substitutions at ordinals 71-149, the ordinal-150 forwarding latch, and the
ordinal-180 finished gate. The bridge recorded no error, overflow, or device
loss. FPS initially stuttered and then recovered to the 60-FPS VSync ceiling;
intermittent stutter remained during play. No historical black/shadow-layer or
solid-color corruption was reported, but this remains a provisional safety
pass until visual correctness is explicitly confirmed.

This was not a controlled performance run. The user clarified that severe
stutter in an object-dense area occurred first and motivated one resolution
change and several HBAO/SSAO changes. The setting changes therefore cannot
explain the initial dense-scene degradation, although they contaminate later
frame-pacing observations. The ending settings were also materially higher
than the early project baseline, including High
subsampling, High shadows and planar reflections, character resolution 2,
particle density 2, view distance 1.37, and 1920 x 1200 fullscreen rendering.

The ESO client log recorded 13 graphics-device/swapchain reset sequences. The
later device-idle intervals were commonly about 15-16 ms, enough to consume
roughly one 60-Hz frame before the rest of the reset work. These resets are
consistent with live resolution and graphics-setting changes and provide a
direct contaminant for stutter around and after those changes. They do not
explain the severe object-dense-area slowdown that preceded the changes. The
macOS window
server initially denied focus while ESO presented no window, then routed
frontmost and keyboard focus to ESO; this recovered launch-focus event is not
evidence that argument buffers caused the focus behavior.

The active MoltenVK pipeline cache changed from
`f271894e906a4177fc90d50dc645ee7efe114e64ceb20b7573a78a7ed3554b48` to
`69b75a8d6866523843a0e2cdfaf8dd5609e649fbf76555940c1935d82f56126a`
and ended at 13,557,217 bytes. Unified logging contained 210 privacy-redacted
Metal compiler warnings, concentrated during startup/world loading with later
sporadic records. This supports pipeline creation/cache population as a
plausible contributor to first-run and intermittent stutter, but neither the
warning count nor the cache rewrite proves causation. The cache is preserved.

## Second installed ESO run

Warm run `20260827T110923.707262000Z-pid88869` used the same exact argument-
buffer candidate for approximately 27 minutes. The user reported no stutter,
but did perceive sustained frame-rate loss and concluded that the raised High
profile was too demanding. ESO traversed Khenarthi's Roost, Vvardenfell, and
Stros M'Kai. The bridge again activated all 17 redirects, suppressed exactly
79 startup draws, latched forwarding at ordinal 150, completed at ordinal 180,
and recorded no error, overflow, or device loss.

The client log contains only the two startup graphics resets at 20:09:24-25
and no loaded-world reset. This cleanly distinguishes the second run from the
13-reset first run and supports reset/setting transitions and cache population
as contributors to the earlier intermittent stutter. It does not prove which
contributor dominated.

The active pipeline cache grew from 13,557,217 to 15,842,531 bytes and changed
to SHA-256
`0e7108f3e1993c62480120ad76e357ee7a1213038fba1ac7741f94d38f18ecdb`.
The ending profile retained 1920 x 1200, High subsampling, High shadows and
planar reflections, character resolution 2, particle density 2, SSAO value 1,
and reduced view distance 1.15. The project has no continuous FPS or GPU-time
sample for this run, so the frame-rate result is correctly recorded as a user-
perceived loss rather than a measured FPS value.

## Medium-to-high profile applied

After ESO exited, the user approved a quality-profile reduction. The complete
live `UserSettings.txt` was preserved as
`UserSettings.txt.teso4m4-before-medium-high-20260827T114452Z` and verified
byte-identical to the pre-change file. Exactly three settings changed:

| Setting | Before | After |
|---|---:|---:|
| `PLANAR_WATER_REFLECTION_QUALITY` | `2` | `1` |
| `PARTICLE_DENSITY` | `2` | `1` |
| `SHADOWS` | `2` | `1` |

The profile retains 1920 x 1200, High subsampling `2`, character resolution
`2`, SSAO `1`, high-resolution shadows `1`, screen-space water reflections
`1`, and view distance `1.14999998`. The backup SHA-256 is
`9a351631881626e0711a389d3aaa1634d20958b853c62c6594d5f3f084d3a344`;
the applied full-file SHA-256 is
`ea4b007281c44144fa55ed15c2e20ff83d35e56913b36d15f32650c855bf862d`.

## Three-run activation/focus regression

The user clarified that all three consecutive argument-buffer starts opened a
visible game window without initially capturing mouse focus. Recovery required
an explicit switch to another application and back to ESO. The exact runs were:

```text
20260827T105710.534190000Z-pid87213
20260827T110923.707262000Z-pid88869
20260827T114657.462339000Z-pid90235
```

All three selected the exact candidate and first recorded ESO's internal
application-active byte as `active=no` with the inactive 100-ms sleep bypassed.
The first two later recorded `active=yes` after the user performed the focus
bounce. The third remained `active=no` until exit, matching the run in which
the user reported the issue and quit without retaining the recovered state.

macOS unified logging independently shows the same sequence on all three
starts. WindowServer first rejected repeated front-process requests because ESO
presented zero windows, then designated ESO frontmost and routed keyboard focus
to its PID once the window existed. Despite that OS-level focal state, ESO's
own AppKit-fed active byte remained false. The evidence therefore identifies an
OS/ESO activation-state divergence rather than an absent visible window or an
ordinary background application.

Confirmed: the focus failure repeated on all three argument-buffer starts and
correlated exactly with an initial false ESO active state. Confirmed: the
existing pacing patch does not modify the AppKit callbacks or active byte, but
its former `focus_event_propagation=unchanged` log wording is only a structural
claim and did not guarantee successful callback delivery.

Inference: Metal argument buffers do not directly call AppKit, but they are the
single runtime variable changed from the 0.1.3 release profile and plausibly
shift startup timing into the repeatable zero-window activation race. The
complete removal of ESO's inactive 100-ms yield may then amplify or preserve
the missed internal activation state. This is an interaction hypothesis, not a
proven direct argument-buffer focus mechanism. One earlier Experiment 0044
argument-buffers-off run also began `active=no` without a reported focus
failure, so the inactive state alone is not sufficient evidence of the symptom.

The repeated input defect rejects the candidate regardless of its descriptor
benchmark or warm-cache stutter result. The first cache-preserving restore
attempt stopped before mutation because the ZeniMax launcher was still running.
After a fresh process check showed both ESO and the launcher exited, the shared
idle gate passed with idle Steam and no ESO file or update activity.

The source log wording was corrected from the disproven propagation guarantee
to `focus_callbacks=unmodified active_byte=observed-only`. This is an
observability correction, not a focus repair. A fresh build passed every C
smoke probe, exact MoltenVK configuration probe, 138 Python tests, Python
compilation, shell syntax, and whitespace checks. The prepared bridge SHA-256
is `9e55acc377a151d1dbe38d406b2d8846054a0630ab1b6ead3e6573dfcb72c660`.

The verified pristine loader was restored and the same freshly built bridge
was installed in `startup-compositor-neutralize-pacing-release` mode. This
retains the 0.1.3 behavior while disabling Metal argument buffers and changing
only the corrected observability wording. Installed and built bridge hashes
match exactly at the SHA-256 above. The medium-to-high settings remained
`ea4b007281c44144fa55ed15c2e20ff83d35e56913b36d15f32650c855bf862d`;
the active and old pipeline caches remained
`036fd84abe8bb3552e9f5960eb643757a97e3614a8f9a9685639e599b86f4de4`
and `72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

## Interpretation

Confirmed: argument buffers materially improve this descriptor-heavy MoltenVK
1.4.2 CPU interval and pass the available generic render/reset coverage.

Confirmed historical risk: MoltenVK 1.4.1 argument buffers were the sole
changed variable strongly implicated in black/shadow-layer flicker. The new
non-game passes cannot prove that ESO's complete descriptor shapes are safe.

The first two installed runs were a provisional rendering-safety pass, not
evidence of a steady-state FPS improvement. The warm run removed the first run's
intermittent stutter without a loaded-world reset, strengthening but not
proving the cache/reset-transient explanation. The candidate did not make the
raised High profile sustain the user's perceived 60-FPS floor in object-dense
areas. Because there is no same-settings argument-buffers-off control, this
does not establish an argument-buffer regression; it establishes that the
candidate's descriptor-path gain is insufficient to overcome this quality
profile's total scene cost.

## Next gate

One ordinary launch of the installed argument-buffers-off behavior now serves
as the focus control. A normal initial
mouse capture supports an argument-buffer timing interaction; another failure
falsifies argument buffers as a sufficient cause and exposes a latent issue in
the release pacing bypass itself. Do not resume the performance A/B until this
P0 activation defect is resolved. A successor pacing experiment should retain
a short inactive yield instead of immediately returning, without forcing the
active byte or synthesizing AppKit focus events.
