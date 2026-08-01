# Experiment 0028: bounded startup draw-input provenance audit

- Date: 2026-08-01
- Outcome: **succeeded; descriptor-state transition isolated**
- Rollback: **pending because Steam remains open after evidence collection**

## Question

When Experiment 0027's sole indexed draw changes the final swapchain image
from canonical magenta to the normal scene without changing its pipeline or
draw signature, which shader-input class changes: descriptor bindings, the
latest descriptor update batch, or push constants?

## Evidence selecting this gate

Experiment 0027 isolated one complete pipeline and indexed-draw identity in
every sampled exact-magenta frame. The same pipeline, vertex/fragment shader
hashes, and draw signature remain in the later normal-scene samples. Earlier
black samples have no draw. This excludes another generic clear, pixel, or
pipeline audit: the remaining distinction is input state or the contents of a
resource reached through stable input state.

Existing ESO proc logs repeatedly obtain `vkUpdateDescriptorSets` and
`vkCmdBindDescriptorSets`; earlier bounded evidence observed 62,160 descriptor
set allocations and 93,707 descriptor updates in eight frames. No existing
log shows descriptor-update-template or push-descriptor entry points. The
candidate therefore covers ESO's observed descriptor path while retaining a
fail-closed capacity substantially above that prior allocation count.

## Candidate

The isolated `startup-input-audit` mode retains the exact effective official
MoltenVK 1.4.2 `performance-aggressive` configuration, Experiment 0026's
twenty pre-present pixel samples, and Experiment 0027's exact draw/pipeline
provenance. It does not change shaders, descriptors, push constants, render
commands, pixels, settings, or pipeline caches.

Only during the existing generation-1/2 ordinal-180 window, it records:

1. bounded descriptor-set-layout identities and counts split into sampled
   images, buffers, samplers, and input attachments;
2. bounded pipeline-layout identities, ordered set-layout identities, and the
   maximum declared push-constant byte range;
3. descriptor-set allocation identity and the latest complete
   `vkUpdateDescriptorSets` call affecting each set, including image view,
   sampler, image layout, buffer, offset/range, and texel-buffer-view handles;
4. the graphics descriptor sets and dynamic offsets bound to each command
   buffer, but at draw time hashes only the set slots required by the bound
   pipeline layout;
5. push-constant call values and ranges as a bounded hash; and
6. the resulting input provenance through the already proven
   command-buffer-to-submit-semaphore-to-present chain.

Repeated identical descriptor-update calls produce the same latest-batch
identity; the audit does not confuse a normal repeated update count with a
state change. A required descriptor set that is missing, has the wrong layout,
or has no captured update is incomplete. A pipeline declaring push constants
without a captured push is also incomplete.

The hard limits are 2,048 descriptor-set layouts, 2,048 pipeline layouts,
131,072 descriptor sets, 16 bound set slots, 4,096 shader modules, 4,096
graphics pipelines, 512 command buffers, 512 signal semaphores, and eight
distinct pipelines per sampled submit. Any overflow, incomplete identity,
missing scheduled pixel, provenance gap, or missing ordinal-180 finish is
`INCONCLUSIVE`.

`tools/analyze_startup_input_audit.py` first repeats the exact Experiment 0027
pipeline isolation. It then requires one stable pipeline-layout identity and
complete draw inputs across all exact-magenta and normal-scene samples:

- a changed descriptor-set handle or latest update batch yields
  `DESCRIPTOR-STATE-CHANGE-CANDIDATE`;
- stable descriptors with changed push values yields
  `PUSH-CONSTANT-CHANGE-CANDIDATE`;
- stable descriptor and push identities with at least one sampled image or
  buffer yields `BOUND-RESOURCE-CONTENT-CANDIDATE`;
- a complete layout without those input classes yields
  `INPUT-PROVENANCE-CAPTURED`, not a false content attribution.

The hashes deliberately do not read texture or buffer memory. A stable
resource-binding result would therefore select a narrower resource-content
audit as the next gate; it would not by itself prove whether an image or
buffer contains magenta.

## Non-game validation

No Steam, launcher, or ESO process was launched. Before the clean build, the
active and backup pipeline-cache hashes and the settings hash were recorded.
The remaining Steam `ipcserver` is parented by PID 1 and holds no ESO bundle
file open; `steam_osx`, the launcher, and ESO are absent.

The synthetic lifecycle probe creates a combined-image-sampler layout,
allocates and updates a descriptor set, declares a 16-byte fragment push
range, binds the set, pushes values, records one indexed draw, submits it with
a signal semaphore, and presents while waiting on that semaphore. The sampled
present retains one required set, one image descriptor, the update identity,
the push identity, and `input_complete=yes`.

The real official MoltenVK 1.4.2/AppKit probe creates and presents a graphics
pipeline with no descriptors or push constants. At both 64 x 66 and ESO's
3420 x 2148 first extent, its exact-magenta draw carries the same complete
zero-input pipeline layout through the real signal semaphore to the
pre-present sample. Corrected surfaces remain opaque black.

Analyzer controls cover stable sampled-image bindings, a changed descriptor
update, and missing input provenance. The last case fails closed.

Validation completed before installation:

```text
clean complete bridge build and BinkOpen re-export: PASS
Rosetta self-patch probe: PASS
Lifecycle trace smoke: startup_input_cases=1 PASS
startup-input-audit MoltenVK configuration probe: PASS
real MoltenVK/AppKit small and exact-ESO surface probe: PASS
114 Python tests: PASS
python compileall, shell syntax, compiler warnings, and git diff check: PASS
```

The normal build script correctly required the original loader. A
cache-preserving restore established that input, and a fresh build directory
was used after the final source change.

## Installation gate

The user's standing direction explicitly authorizes the necessary diagnostic
installation once a discriminating candidate has passed its static and
non-game gates. Those gates now pass. Before installation, commit this exact
source checkpoint; then install only `startup-input-audit` with both caches
preserved, verify installed/build bytes and the three pre-install hashes, and
establish a fresh evidence boundary.

Only after those checks pass is one user-controlled normal Steam-path startup
justified. The user only needs to report whether the pink frame appeared; the
dedicated analyzer will classify the exact run. Restore
`performance-aggressive` immediately after evidence collection.

## Approved installation checkpoint

After source checkpoint `4ee8194`, the process gate found neither `steam_osx`,
the launcher, nor ESO. The remaining Steam `ipcserver` had parent PID 1 and no
open ESO bundle file. A cache-preserving restore and fresh full source build
preceded installation of only `startup-input-audit`.

The installed proxy, renamed original, and official MoltenVK match the clean
build byte for byte:

```text
proxy SHA-256:     80a34e90ac4be479f02ab57cf131e1185ed30010c5e826cbc7734193ac7bbc9e
MoltenVK SHA-256:  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
mode marker:       startup-input-audit
```

Installation preserved both caches and settings exactly. The active cache is
`a9a4ee5112466265c233e2561bb6c284032bfa30f077c0227259f09db682a063`,
the old-backup cache is
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`,
and `UserSettings.txt` is
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.
The update check is `CURRENT` for ESO 12.0.7/databuild `3281538`; all three
pipeline-cache UUID checks pass.

The ignored evidence boundary
`artifacts/experiment-0028-20260801T153500Z` began at
`2026-08-01T11:46:03Z` from source commit `4ee8194`. Its launcher snapshot was
1,796 seconds old, matched all eight repositories, and reported
`noUpdateRequired`, within the fixed 3,600-second gate. No Steam, launcher, or
ESO process was launched by the agent. One user-controlled normal Steam-path
startup is now justified; its evidence and restoration remain pending.

## Result

The user completed one normal Steam-path startup and reported that the pink
frame remained visible. Exact run `20260801T160428.543259000Z-pid786`
completed all twenty scheduled pixel, draw, pipeline, and input summaries
through generation-2 ordinal 180. There was no skipped pixel read, lifecycle
error, table overflow, pipeline overflow, input-provenance gap, or crash
report.

The nine samples through generation-2 ordinal 70 remained opaque black with
no draw. Ordinals 80 through 140 were exact RGBA `(255,0,255,255)` and used
the same one indexed draw and pipeline from Experiment 0027. Ordinals 150
through 180 contained normal scene colors with that draw and pipeline still
unchanged.

The target pipeline layout is `d175d2c1daed112d`. It has two ordered
descriptor-set layouts and no push-constant range:

```text
set 0: e3c2499a89df1706, 3 descriptors, 0 images, 3 buffers
set 1: d0edad262f8c4230, 5 descriptors, 2 images, 3 buffers
total: 8 descriptors, 2 images, 6 buffers, push bytes 0
```

Across all seven magenta and four normal-scene samples, the required set count,
pipeline layout, bound descriptor-set handles, and absence of push constants
were stable. Only the latest descriptor-update state changed:

```text
bound sets:                  2 -> 2
layout signature:            53353b05ed5272e3 -> 53353b05ed5272e3
descriptor-handle signature: 28fac876bb1f8a25 -> 28fac876bb1f8a25
descriptor-update signature: 01922f8394b93e32 -> a7d448d22e640458
push signature:              0000000000000000 -> 0000000000000000
```

The dedicated analyzer verdict is
`DESCRIPTOR-STATE-CHANGE-CANDIDATE`. This excludes a push-constant transition
and weakens a pure in-place resource-content transition: ESO changes an image
view/sampler/layout, buffer handle/offset/range, texel-buffer view, or another
captured descriptor update while retaining the same two descriptor-set
objects. The current aggregate does not yet identify whether the changing
update belongs to the buffer-only set 0 or the mixed image/buffer set 1.

The next discriminating gate is a per-required-set, per-descriptor-class split
for this exact pipeline. It should identify image versus buffer state without
reintroducing the broad render audit. No new user run is justified until that
bounded successor passes static, synthetic, and real non-game validation.

The generic startup checker reported `FAIL` only because it does not recognize
this isolated diagnostic mode as a supported compatibility mode. The dedicated
analyzer independently required the exact mode/configuration, twenty aligned
samples, complete input provenance, and bounded finish before returning its
verdict.

Evidence collection preserved the settings at
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c` and
the old-backup cache at
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.
The active 1.4.2 cache updated normally to
`234dc3189fcd2156e9de984a8aec5d5b87a66e8a6e39f7b4f081df851019a7b8`.
ESO is closed, but Steam PID 429 remains open, so the safety gate correctly
defers the cache-preserving rollback until the user exits Steam completely.
