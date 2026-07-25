# Experiment 0015: reset-only pipeline-cache bypass

- Date: 2026-07-25
- Outcome: **planned**
- Rollback: **not started**

## Question

Does MoltenVK 1.4.1's use of ESO's warmed pipeline cache cause
rendering-incorrect graphics pipelines during a loaded-world device reset?

## Basis

Experiment 0014 observed an initial 8 FPS state in which graphics options could
be reset freely, followed after a complete Steam-path restart by a 60 FPS state
in which changing resolution again produced solid color. Three bounded audits
had complete state coverage and found no stale descriptor set rebound, live
image overlap, tracked layout mismatch, dead attachment, or
pipeline/render-pass mismatch.

Every reset-created graphics pipeline in all three audits received ESO's active
pipeline cache. The cache grew by 55,314 bytes and changed hash during the
multi-run interval. This does not prove cached pipeline corruption, but it is
the narrowest remaining Vulkan-level counterfactual.

## Exact change and controls

- Keep MoltenVK 1.4.1, all 17 redirects, both HDR filters, live-resource
  checking, disabled Metal argument buffers, MTLHeap `where safe`, synchronous
  submission, command pooling enabled, and command-buffer prefill disabled.
- Keep the lifecycle, reset-resource, and render-graph audits unchanged.
- Keep ordinary startup, character selection, world loading, and steady-world
  `vkCreateGraphicsPipelines` calls unchanged.
- Only while the existing loaded-world reset audit is active, replace the
  optional `VkPipelineCache` argument to `vkCreateGraphicsPipelines` with
  `VK_NULL_HANDLE`.
- Preserve all other create-info fields, output pointers, return values, and
  call order.
- Do not delete, rename, truncate, or replace either pipeline-cache file.

Vulkan permits a null pipeline cache. This change may increase the one-time
pipeline compilation cost during graphics reset, but it does not intentionally
disable steady-world cache use or reduce normal rendering performance.

## Instrumentation gate

The bridge logs each bypass with the requested non-null cache, forwarded null
cache, pipeline count, and result. The render-audit analyzer requires:

1. at least one bypass record;
2. the sum of bypassed pipelines to equal every graphics pipeline created in
   the bounded reset window;
3. zero reset pipelines reported as created with a cache;
4. no mirror overflow or Vulkan failure.

## Procedure

The target state is the recovered 60 FPS phase:

1. launch through Steam and enter the existing world;
2. if the known 8 FPS state appears, do not change a setting; exit Steam
   completely and relaunch once;
3. once ordinary approximately 60 FPS behavior is present, change fullscreen
   resolution once from 2048 x 1280 to 1920 x 1200;
4. wait only long enough to classify correct output, solid color, frozen last
   frame, or crash, then exit.

No travel, HUD, capture, FPS counter, or second option change is required.

## Pass/fail

- **Pass:** the 60 FPS phase remains visually correct through the resolution
  reset and the analyzer proves every reset pipeline bypassed the cache.
- **Fail:** solid color/frozen output recurs despite a complete bypass, or ESO
  crashes.
- **Inconclusive:** only the 8 FPS state can be reached, no replacement
  pipeline is created, or the bypass coverage is incomplete.

## Evidence

Pending source validation and installation.

## Result

Pending.

## Interpretation

Pending.

## Rollback

Not started. The checked pristine loader and both pipeline-cache files remain
the restore boundary.

## Follow-up

A pass justifies a narrow reset-only workaround candidate and reset-latency
measurement. A fail ends pipeline-cache work and moves to descriptor ordering
or Metal-side object/content capture.
