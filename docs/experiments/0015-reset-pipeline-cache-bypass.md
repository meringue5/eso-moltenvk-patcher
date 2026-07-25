# Experiment 0015: reset-only pipeline-cache bypass

- Date: 2026-07-25
- Outcome: **running; installed and awaiting the targeted warm-state reset**
- Rollback: **not performed; restore path checked**

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

Source commit `cf0261f` passes 65 Python tests, Python compilation, shell
syntax, whitespace checks, warnings-as-errors, and Clang static analysis. The
complete pristine-loader build passed Bink re-export, Rosetta self-patch, HDR,
lifecycle, reset-resource, render-audit, and all seven effective-configuration
probes.

The reset smoke probe proves the scope boundary: with bypass enabled, a
pre-reset pipeline receives its original non-null cache, while an active reset
pipeline receives `VK_NULL_HANDLE` and emits the required bypass record. The
render-audit analyzer tests accept exact bypass coverage and reject a pipeline
count mismatch.

Fresh replacement and embedded-runtime probes reached the Apple M4 and
reproduced the established HDR, surface-format, device-creation, and 100-name
proc-address results.

With ESO, Steam, and the launcher stopped, all Experiment 0014 evidence
checksums were verified. A cache-preserving restore returned the loader to its
pristine source, a fresh build completed, and the candidate was installed in
exact `reset-no-pipeline-cache` mode. Built and installed hashes match:

```text
libBink2Macx64.dylib
9bd7ca5f227bf7a4c393b064df098a19f2bdd7c5845b739658b17b73c4bb02bc

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

The 4,259,071-byte warmed active cache, 6,800,792-byte old backup, and exact
settings file remain unchanged. Status reports the target and marker current,
and the quick gate is `READY`. Fresh ignored evidence is prepared under
`artifacts/experiment-0015-20260725T131534Z`. No agent launched Steam, the
launcher, or ESO.

## Result

Pending.

## Interpretation

Pending.

## Rollback

Not performed. The checked pristine loader and both unchanged pipeline-cache
files remain the restore boundary.

## Follow-up

A pass justifies a narrow reset-only workaround candidate and reset-latency
measurement. A fail ends pipeline-cache work and moves to descriptor ordering
or Metal-side object/content capture.
