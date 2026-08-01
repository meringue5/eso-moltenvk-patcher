# Experiment 0028: bounded startup draw-input provenance audit

- Date: 2026-08-01
- Outcome: **candidate passed the non-game gate; approved installation pending**
- Rollback: **original loader restored for the clean build; installation not yet performed**

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
