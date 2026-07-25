# Experiment 0012: bounded reset-resource trace

- Date: 2026-07-25
- Outcome: **failed rendering correctness; trace localized the failure beyond
  swapchain and memory binding**
- Rollback: **not performed; restore path rechecked before installation**

## Question

Which non-swapchain resource or command path changes during the loaded-world
reset that continues presenting invalid content?

## Basis

Experiments 0010 and 0011 exclude the tested swapchain-result,
presentation-scaling, and MTLHeap hypotheses. The failed frame is produced
before a functioning presentation. Further configuration changes would not
identify the failing object.

## Exact change and controls

- Restore MTLHeap to `where safe`; Experiment 0011 proved that disabling it
  does not repair this failure.
- Keep official MoltenVK 1.4.1, disabled Metal argument buffers,
  live-resource checking, synchronous submission, command pooling, both HDR
  filters, all 17 redirects, and the existing lifecycle trace.
- Add observation-only wrappers to both ESO's direct static resource targets
  and its GDPA-returned functions.
- Forward every Vulkan argument and result unchanged.

The trace arms only on the first successful `vkDeviceWaitIdle` after two
swapchains have been established. This matches the observed live-reset
boundary. It ends after eight presentations on the replacement swapchain.
The eight-presentation bound is a log-volume limit and is unrelated to the
earlier one-run 8 FPS symptom, which did not reproduce on the next launch.

During that bounded window it counts:

- buffer/image creation, destruction, memory requirements, binding, mapping,
  allocation, and freeing;
- image views, render passes, and framebuffers;
- descriptor pools, sets, layouts, writes, copies, and resets;
- pipeline layouts and graphics/compute pipeline creation;
- command-buffer allocation, recording completion, render-pass completion,
  queue submission, and available draw/dispatch bindings.

The first 48 high-value allocation, image, memory-requirement, pipeline, and
queue-submit records include compact parameters. Negative Vulkan results are
always logged. A coded analyzer requires exactly one begin, one replacement
swapchain, one summary, and the complete counter schema.

## Non-game gate

Before requesting a run:

1. compile with warnings as errors and static analysis;
2. pass the chained lifecycle/reset forwarding probe;
3. verify every resource-related direct patch target is intercepted;
4. keep inactive high-frequency wrapper overhead below 1 microsecond;
5. pass Python, analyzer, shell, and whitespace checks;
6. rebuild from a committed source state against the pristine loader;
7. rerun the real-Metal HDR and surface probes;
8. obtain explicit approval before restoring or installing any game-bundle
   file.

## User action

The installation gate passed. Perform one Steam-authenticated run:

1. enter the existing character's world and confirm initial rendering;
2. change fullscreen resolution once from the current 2048 x 1280 to any other
   available value and apply it;
3. stop immediately after correct rendering is visible or persistent
   corruption appears.

Do not change ambient occlusion or another setting. No HUD, capture, FPS report,
travel, or exploratory play is required. Report only the initial and
post-resolution-change visual state.

## Result

Source commit `a1714da` passed 51 Python tests, Python compilation, shell
syntax, whitespace checks, warnings-as-errors, and Clang static analysis. The
chained lifecycle/reset probe passed and measured the inactive high-frequency
draw wrapper at 4 ns per call, below its 1 microsecond safety ceiling. The probe
also verifies that every resource-related direct patch target is intercepted,
that suboptimal presentation is forwarded unchanged, and that the bounded
counter summary is complete.

A clean full build used a temporary app structure pointing at the pristine
loader, so no installed game-bundle file was changed. It passed Bink re-export,
Rosetta self-patch, HDR, lifecycle, reset-resource, and all four configuration
probes. `reset-resource-trace` reports MTLHeap `1` with the other established
compatibility values unchanged.

Fresh legacy and replacement-runtime probes reached the real M4 Metal device.
The candidate hid the HDR extension, removed the one exact HDR surface pair,
created the non-HDR device, and preserved the discrete descriptor path.

Prepared artifact hashes are:

```text
libBink2Macx64.dylib
c4f7110a1d1e7c1f6e10df643fc4698d4fcc73e95ce8717f0a54b2cd772a11f9

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

The user explicitly approved installation. Experiment 0011's evidence was
already checksum-preserved. A cache-preserving restore returned only the loader
to pristine state, after which commit `a1714da` was rebuilt again against the
real bundle and passed the complete build gate. Experiment 0012 was installed
in `reset-resource-trace` mode.

Installed and built hashes match the prepared values above. The marker contains
exactly `reset-resource-trace`, the target is current, and the quick update gate
is `READY`. Both pipeline caches and user settings were preserved.

Fresh evidence is prepared under the ignored
`artifacts/experiment-0012-20260725T115444Z` directory. The active settings
SHA-256 is
`f2ea9f42d300a123744ff1ee9b4266ad8f4963653cded75ad5e984c184bdcc5f`
with ambient occlusion `1`, pregame skipping `1`, and the fullscreen resolution
left by Experiment 0011 at 2048 x 1280. The 4,190,143-byte active cache has
SHA-256
`58e3474cc67d240ad06d27e79316fcac0203d7c9b81e3f0d6072765f2d8d6679`;
the old cache remains unchanged.

No agent launched Steam, the launcher, or ESO.

The user entered a correctly rendered world and changed fullscreen resolution
once from 2048 x 1280 to 1920 x 1200. Persistent solid-color output recurred.
The settings diff is structurally exact and contains only those width and
height changes. The bridge startup verdict passed, no crash report was created,
and ESO's three reset sequences contained no error marker.

The bounded trace completed after eight presentations on replacement
swapchain generation 3 with zero Vulkan failure records. During those eight
frames ESO submitted eight command buffers containing 484 balanced render
passes, 2,232 pipeline binds, 7,699 descriptor-set binds, and 7,699 indexed
draws. It created 119 graphics pipelines, 65 images, 67 image views, 53
buffers, 50 render passes, and 50 framebuffers. The trace also observed 456
descriptor-set allocations and 93,707 calls to `vkUpdateDescriptorSets`.

There was no `VkDeviceMemory` allocation, free, map, or unmap in the bounded
window. All 65 images and 53 buffers were instead bound successfully into
existing allocations. A coded follow-up reconstructed the 15 complete image
bindings present within the 48-detail cap: all offsets met their reported
128-byte alignment, no two captured new-image ranges overlapped, and the final
create record alone was truncated by the detail cap. The generated result was
added to the evidence manifest, and all 43 checksums pass.

The active pipeline cache grew from 4,190,143 to 4,203,757 bytes and changed
hash. The old preserved cache remained byte-identical. The installed
Experiment 0012 bridge remains active; rollback was not required for evidence
preservation.

## Interpretation

Confirmed observations exclude a stopped renderer, an unsubmitted replacement
swapchain, an exposed Vulkan failure, new memory-allocation failure, simple
misalignment, and overlap among the captured new image bindings. Rendering
work continues at high volume while the presented content is wrong.

The strongest remaining region is the descriptor/resource-state transition.
MoltenVK 1.4.1's official source states that its new descriptor implementation
is less forgiving of destroyed descriptor targets. The enabled live-resource
compatibility mode turns each relevant binding into a liveness check and skips
a non-null texture or buffer that is no longer live. MoltenVK 1.0.18 instead
materializes Metal texture and buffer objects into its descriptor binding at
update time and has neither that state tracker nor the compatibility switch.
Together with 77 destroyed image views, 67 replacements, and 93,707 descriptor
updates during the failed boundary, this supports a descriptor-state
compatibility hypothesis. It does not yet prove which descriptor was wrong.

The next single-variable counterfactual is to retain all established
compatibility controls and disable MoltenVK command pooling. No command buffer
was allocated or freed in the failed reset window even though eight were
submitted, so forcing command objects to be allocated and destroyed instead of
reused directly tests stale pooled resource state. It may increase CPU
overhead and is a correctness experiment, not a performance recommendation.
