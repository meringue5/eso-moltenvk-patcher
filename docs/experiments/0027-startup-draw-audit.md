# Experiment 0027: bounded startup presented-draw audit

- Date: 2026-08-01
- Outcome: **running; approved audit installed and fresh evidence boundary prepared**
- Rollback: **pending the single user-controlled startup; restore path checked**

## Question

Which graphics-pipeline and draw signature is present in final swapchain frames
that are exact magenta, but absent from the preceding opaque-black frames?

## Evidence selecting this gate

Experiment 0026 directly read five points from twenty final swapchain images.
The exact user run changed from opaque black through generation-2 ordinal 70 to
exact RGBA `(255,0,255,255)` at ordinals 80 through 140, then to normal scene
colors at ordinals 150 through 180. The magenta therefore exists before
`vkQueuePresentKHR`. Experiment 0024 shows that every submitted full-surface
clear in the same bounded interval is opaque black.

The Experiment 0026 proc trace obtains descriptor and pipeline-layout entry
points, `vkCreateGraphicsPipelines`, pipeline binding, viewport/scissor,
vertex/index binding, and `vkCmdDrawIndexed` immediately after the ordinal-70
black sample. The first sampled magenta frame is ordinal 80. A
swapchain-linked draw/pipeline association is therefore more discriminating
than another material-name or post-swapchain hypothesis.

## Candidate

The isolated `startup-draw-audit` mode retains the exact effective official
MoltenVK 1.4.2 `performance-aggressive` configuration and Experiment 0026's
twenty pre-present pixel samples. It does not change render commands, shaders,
descriptors, pixels, settings, or caches.

While the generation-1/2 ordinal-180 window is open, the lifecycle wrapper:

1. hashes each `VkShaderModuleCreateInfo` byte sequence with a bounded 64-bit
   fingerprint and records only size and hash, never shader code;
2. gives each graphics pipeline a stable signature derived from its shader
   hashes and relevant pipeline state, retaining separate vertex and fragment
   hashes;
3. records graphics-pipeline binds and draw/draw-indexed parameters only for a
   command buffer after it enters a swapchain-linked render pass;
4. aggregates the ordered draw signature and at most eight distinct pipeline
   signatures when that command buffer is successfully submitted;
5. copies that exact provenance into each signal semaphore and invalidates it
   if another submit consumes the semaphore;
6. at each scheduled pre-present sample, requires same-queue semaphore
   provenance and logs the submitted draw/pipeline/shader identity before
   reading the five final pixels.

The tables are bounded at 4,096 shader modules, 4,096 graphics pipelines, 512
command buffers, and 512 signal semaphores. A table overflow, more than eight
distinct pipelines in one sampled submit, an untracked shader, missing
semaphore provenance, missing pixel, or missing ordinal-180 finish is
`INCONCLUSIVE`.

`tools/analyze_startup_draw_audit.py` classifies frames from their pixels, so a
startup timing shift does not invalidate the comparison. It reports the stable
pipeline signatures common to all sampled exact-magenta frames and absent from
all sampled opaque-black frames. Exactly one such signature yields
`DRAW-PIPELINE-CANDIDATE-ISOLATED`; otherwise a complete trace yields
`DRAW-PROVENANCE-CAPTURED`, not a false causal claim.

## Non-game validation

No Steam, launcher, or ESO process was launched. The installed game bundle,
normal marker, active and backup pipeline caches, and user settings were not
changed.

The synthetic lifecycle probe creates two distinct shader modules and a
graphics pipeline, records one indexed draw into a swapchain-linked command
buffer, submits it with a signal semaphore, and presents while waiting on that
semaphore. The scheduled pixel callback receives exactly:

```text
matched_signals=1 tracked_commands=1
draw_count=1 indexed_draw_count=1
distinct_pipelines=1 pipeline_overflow=no
shader_hash_complete=yes pipeline_state=tracked
```

It also retains the earlier consumed-semaphore rejection. The analyzer's
positive control isolates one magenta-only pipeline, while pipeline overflow
and incomplete shader identity both fail closed.

The real official MoltenVK 1.4.2/AppKit probe then renders canonical magenta
with a real vertex/fragment pipeline. At both 64 x 66 and ESO's exact
3420 x 2148 first extent, the same pre-present sample contains five exact
magenta points and the same submitted frame contains one draw, one stable
pipeline signature, and complete vertex/fragment shader hashes. The corrected
64 x 64 and 3420 x 2146 surfaces remain opaque black. The probe passes under
the exact aggressive MoltenVK configuration.

Additional validation:

```text
temporary complete bridge link and BinkOpen re-export: PASS
Lifecycle trace smoke: startup_draw_cases=1 PASS
startup-draw-audit MoltenVK configuration probe: PASS
111 Python tests: PASS
python compileall, shell syntax, and git diff check: PASS
```

The normal build script correctly refuses to treat an installed bridge as its
original Bink input. A complete changed proxy was therefore linked under
`/tmp` against the already verified renamed original for this non-game gate.
A clean cache-preserving restore and source rebuild remain part of the explicit
installation gate.

## Installation gate

Installing `startup-draw-audit` requires new explicit approval naming that
exact mode. Prior approvals do not apply. After approval, verify Steam, the
launcher, and ESO are closed; perform a cache-preserving restore; rebuild from
source; install with both caches preserved; verify installed/build bytes,
settings, and cache hashes; then establish a fresh launcher/evidence boundary.

Only after all of those checks pass is one user-controlled normal Steam-path
startup justified. The user only needs to report whether the pink frame was
visible; the analyzer then uses the same run's exact pixels and draw
provenance. Restore `performance-aggressive` immediately after evidence
collection.

## Approved installation checkpoint

The user explicitly approved `startup-draw-audit`. With `steam_osx`, the
launcher, and ESO absent, the remaining parentless Steam `ipcserver` was
verified not to hold an ESO bundle file open. A cache-preserving restore then
re-established the pristine loader, followed by a clean source build and the
approved experimental installation.

The installed proxy and official MoltenVK match the clean build byte for byte:

```text
proxy SHA-256:     03988f2df27da9ce653087ec4b2f30a4b53de6ab7e09b0117c273e79621e0a27
MoltenVK SHA-256:  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
mode marker:       startup-draw-audit
```

The install preserved both caches and settings exactly. The active cache is
`8b061e93fa21d2b687ec7eef5bafa363c04418e468928bdaee3a687585773de7`,
the old-backup cache is
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`,
and `UserSettings.txt` is
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.
The update and launcher checks report `READY` for ESO 12.0.7/databuild
`3281538`.

The fresh ignored evidence boundary
`artifacts/experiment-0027-20260801T111424Z` started at
`2026-08-01T11:14:26Z` from source commit `b29e6dd`. No game process was
launched by the agent. Exactly one user-controlled normal Steam-path startup
is now authorized; evidence collection and immediate restoration remain
pending.
