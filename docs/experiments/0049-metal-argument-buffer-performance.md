# Experiment 0049: Metal argument-buffer performance candidate

- Date: 2026-08-27
- Outcome: **installed; non-game gates passed and user rendering gate pending**
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

## Interpretation

Confirmed: argument buffers materially improve this descriptor-heavy MoltenVK
1.4.2 CPU interval and pass the available generic render/reset coverage.

Confirmed historical risk: MoltenVK 1.4.1 argument buffers were the sole
changed variable strongly implicated in black/shadow-layer flicker. The new
non-game passes cannot prove that ESO's complete descriptor shapes are safe.

## Next gate

Classify the installed user run first. Only a rendering-correct pass advances
to a fixed-scene frame-time A/B. Any visual corruption rejects the candidate
and triggers cache-preserving restoration of the public 0.1.3 profile.
