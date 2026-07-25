# Experiment 0012: bounded reset-resource trace

- Date: 2026-07-25
- Outcome: **planned; source candidate under non-game validation**
- Rollback: **not performed; failed Experiment 0011 checkpoint installed**

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

Not yet requested. If installation is approved and every non-game gate passes,
the only user action will be the same one-change reproduction: enter the world,
change fullscreen resolution once, and stop on correct rendering or persistent
corruption. No HUD, capture, FPS report, or exploratory play will be required.

## Result

Pending.
