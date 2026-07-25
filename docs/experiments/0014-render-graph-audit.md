# Experiment 0014: bounded render-graph audit

- Date: 2026-07-25
- Outcome: **planned**
- Rollback: **not started; restore path checked**

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

Pending build, installation, and user-controlled run.

## Result

Pending.

## Interpretation

Pending.

## Rollback

Not performed. Installation will retain a checked pristine-loader restore path
and preserve both pipeline caches in place.

## Follow-up

Do not request another user run until the coded analyzer has classified this
one.
