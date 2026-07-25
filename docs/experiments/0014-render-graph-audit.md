# Experiment 0014: bounded render-graph audit

- Date: 2026-07-25
- Outcome: **failed rendering correctness; diagnostic audit succeeded**
- Rollback: **not performed; restore path checked**

## Question

Can one observation-only reset run distinguish the five remaining
offscreen-to-composite fault categories without another sequence of generic
configuration A/B tests?

## Hypotheses covered together

1. A bound descriptor set still contains an image view after that view is
   destroyed, or a dead view is written after reset.
2. Descriptor updates, copies, or rebinds leave the replacement final-pass
   set incomplete or stale.
3. A newly bound image overlaps a still-live image in the same
   `VkDeviceMemory` range without declared aliasing.
4. A pipeline barrier or render-pass attachment begins from a layout that
   differs from the observed layout history.
5. Reset-created graphics pipelines are not rebound to the recreated
   render-pass chain as expected.

These are correlated hypotheses, not five independent game runs.

## Target and change set

- Keep official MoltenVK 1.4.1 and all 17 verified redirects.
- Keep both HDR filters, live-resource checking, Metal argument buffers
  disabled, MTLHeap `where safe`, synchronous submission, command pooling
  enabled, and command-buffer prefill disabled.
- Keep the existing lifecycle and eight-presentation reset-resource window.
- Add a fixed-capacity descriptor/image-view mirror beginning with the first
  swapchain.
- During the existing bounded window, correlate destroyed image views,
  descriptor writes and binds, live and recently destroyed image-memory
  ranges, image barriers, render-pass attachments, pipeline creation/binding,
  and image copy/blit/resolve calls.
- Do not mutate Vulkan arguments, return values, resources, layouts,
  descriptors, or submission order.

The layout audit observes API recording order. A layout mismatch is a lead to
correlate with the command-buffer sequence, not by itself proof of an invalid
executed transition.

## Preflight

The current ESO 12.0.7 executable hash, UUID, client version, and databuild
still match the selected target. The installed Experiment 0013 bridge is
current. Its negative result and checksum-preserved evidence remain intact.

The new mirror uses bounded, allocation-free tables. Descriptor-set and
image-view hot lookups use open-addressed hashing, and descriptor-pool
invalidation scans slots once per operation rather than once per set. The
non-game probe measures the repeated descriptor-mirror path and rejects a
10-microsecond-per-update regression.

Before installation:

1. pass all Python tests, Python compilation, shell syntax, whitespace,
   warnings-as-errors, and Clang static analysis;
2. pass the reset wrapper and render-audit smoke probes;
3. rebuild from source against the pristine loader;
4. pass all six effective-configuration probes and both real-Metal
   compatibility probes;
5. preserve the current settings and both pipeline caches.

## Procedure

After installation, the only user action is:

1. launch ESO through Steam and enter the existing world;
2. change fullscreen resolution once;
3. report whether output remains correct, becomes a solid color, freezes on
   the last frame, or crashes;
4. stop there.

No travel, HUD, capture, FPS measurement, or second graphics setting change is
needed. The agent will collect and classify the audit after the run.

## Pass/fail and evidence gate

- **Instrumentation pass:** exactly one audit begins and completes after eight
  replacement-swapchain presentations, its mirror reports no capacity
  overflow, and the startup/configuration verdict passes.
- **Diagnostic signal:** at least one stale descriptor bind, dead referenced
  view, undeclared live image overlap, tracked layout mismatch, or anomalous
  reset-pipeline linkage is correlated to the bounded window.
- **Rendering pass:** the replacement frames remain visually correct after the
  resolution change.
- **Rendering fail:** solid-color/frozen output or a crash recurs.

If rendering fails without any mirror overflow, the relative counters and
sequence records will rank a descriptor-lifetime repair, a layout/barrier
repair, or deeper Metal-side capture. This experiment is diagnostic; it is not
described as a fix.

## Evidence

Source commit `28d9bce` passes 61 Python tests, Python compilation, shell
syntax, whitespace checks, warnings-as-errors, and Clang static analysis. The
complete pristine-loader build passed Bink re-export, Rosetta self-patch, HDR,
lifecycle, reset-resource, render-audit, and all six effective-configuration
probes. The render-audit probe reproduced stale descriptor, live image overlap,
destroyed-range reuse, two layout mismatches, exact pipeline/render-pass
linkage, and all three image transfer paths. Its 100,000-iteration descriptor
mirror benchmark measured 33 nanoseconds per update.

Fresh real-Metal probes reached the Apple M4. MoltenVK 1.4.1 retained its HDR
advertisement, 60 surface formats including the ESO HDR pair, successful
device creation, and the expected 100-name proc comparison. Embedded MoltenVK
1.0.18 retained no HDR advertisement and its three non-HDR formats.

With ESO, Steam, and the launcher confirmed stopped, the prior bridge was
restored using the checked pristine loader while both pipeline caches remained
in place. A fresh build from the committed source was installed in
`render-audit` mode. Built and installed hashes match:

```text
libBink2Macx64.dylib
a6d3fdd4068de41ccaa88029564d0b98f1e14564e9881e3bf641f29636245304

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

The marker contains exactly `render-audit`; status reports the bridge target
current and enabled; the quick update gate is `READY`. Command pooling is back
at the 1.4.1 default in this mode. The exact settings file and both pipeline
caches were preserved.

Fresh ignored evidence is prepared under
`artifacts/experiment-0014-20260725T125558Z`. No agent launched Steam, the
launcher, or ESO.

## Result

The user observed two distinct operating states under the same installed
candidate:

1. an initial 8 FPS state in which resolution and other graphics options could
   be changed without corrupting output;
2. after exiting through Steam and restarting the whole launch path, a restored
   60 FPS state in which changing resolution again produced persistent solid
   color.

Four post-preparation ESO processes began an audit. One ended after the audit
armed but before a replacement swapchain completed the window. Three completed
all eight presentations with no Vulkan failure or mirror overflow. The
chronology and user report place the terminal run
`20260725T130615.206232000Z-pid43186` in the 60 FPS/solid-color phase; the logs
do not themselves measure FPS.

The coded all-runs comparison reports:

| Metric | Earlier complete run 1 | Earlier complete run 2 | Terminal 60 FPS/fail run |
|---|---:|---:|---:|
| Descriptor update calls | 8,690 | 8,495 | 9,671 |
| Image descriptor writes | 50,433 | 49,380 | 60,182 |
| Referenced views destroyed | 3,632 | 2,723 | 512 |
| Stale descriptor sets bound | 0 | 0 | 0 |
| Destroyed image ranges reused | 244 | 307 | 117 |
| Live image overlaps | 0 | 0 | 0 |
| Layout mismatches | 0 | 0 | 0 |
| Reset graphics pipelines | 135 | 124 | 151 |
| Reset pipelines created with cache | 135 | 124 | 151 |
| Exact pipeline/render-pass binds | 2,359 | 2,142 | 1,632 |
| Different render-pass binds | 0 | 0 | 0 |

In the terminal run, all 151 reset-created graphics pipelines used a non-null
pipeline cache, all 1,632 observed graphics binds used a pipeline created in
the same audit, and every bind matched the exact active render-pass handle.
The first 128 render-pass tails contained live descriptor sets with zero stale
slots. Image barriers numbered 77 with no tracked layout mismatch. No image
copy, blit, or resolve path was used.

The terminal run continued through 484 balanced render passes and 9,671
indexed draws in the first eight replacement frames. Swapchain lifecycle was
clean, the startup verdict passed, no crash report appeared, and all evidence
checksums pass. The settings file ended byte-identical to its baseline, so the
user's final resolution selection was not persisted at exit. The active
pipeline cache grew from 4,203,757 to 4,259,071 bytes and changed hash; the old
backup remained unchanged.

## Interpretation

**Confirmed:** destroying an image view while mirrored descriptor slots still
name it is common during ESO's reset, but none of those stale sets was
subsequently bound in any completed audit. The simple form of hypothesis 1—a
destroyed view directly rebound into the final pass—is not supported.

**Confirmed:** no completed run showed an unknown descriptor handle, live
image-memory overlap, tracked barrier/render-pass layout mismatch, attachment
with a dead view, pipeline bound against a different render pass, or mirror
overflow. These observations substantially reduce hypotheses 3 and 4 and the
render-pass-linkage form of hypothesis 5. They do not prove Metal-side resource
contents correct.

**Inference:** destroyed-range reuse is normal in both behavioral states and
was more frequent in the earlier completed runs than in the terminal corrupt
run. Its event count alone therefore does not explain corruption.

**Leading hypothesis:** pipeline-cache contents now provide the narrowest
testable difference. The low-performance state permitted live resets; after
the complete launch-path restart, performance returned to 60 FPS and the reset
failed. Every replacement pipeline in all completed audits was created with
the active cache, which changed and grew over the session. This is correlation,
not proof of a bad key or cached Metal pipeline.

**Still open:** a descriptor rebind/order semantic not expressible as a dead
handle, and a Metal-side lifetime/content hazard after legal Vulkan object
destruction.

## Rollback

Not performed. The checked pristine-loader restore path remains available and
both pipeline caches were preserved in place.

## Follow-up

Prepare one targeted counterfactual: during the bounded loaded-world reset
only, forward `vkCreateGraphicsPipelines` with `VK_NULL_HANDLE` instead of
ESO's pipeline cache. Vulkan permits a null cache. Keep ordinary startup and
steady-world pipeline creation unchanged. If this preserves 60 FPS and makes
the resolution reset render correctly, the cache path is causal enough to
design a durable reset-only workaround. If it fails, move to descriptor update
ordering or Metal-side capture rather than another configuration toggle.
