# Experiment 0016: full-lifetime reset-state audit

- Date: 2026-07-25
- Outcome: **running; installed and awaiting one targeted reset**
- Rollback: **not performed; restore path checked**

## Question

Which remaining public Vulkan state category, if any, becomes inconsistent
across ESO's loaded-world graphics reset when descriptor contents are known
from process startup and command execution is evaluated in queue-submit order?

## Hypothesis

At least one of these joined observations will distinguish the failure:

1. a pre-swapchain descriptor set retains an unknown, dead, or unexpectedly
   updated image, sampler, buffer, or buffer-view slot;
2. ESO submits a command-buffer generation whose reset, begin, end, descriptor
   bind, or descriptor update order is inconsistent;
3. a render-pass attachment has an incompatible load/store/clear or
   first-use/last-use relationship;
4. an aspect/mip/layer transition has an incompatible layout, stage, or access
   relationship in queue execution order;
5. a submit waits on an untracked semaphore or uses an inconsistent fence
   sequence.

If complete fixed-table coverage finds none of those signals while rendering
still fails, the remaining category is below the observed public Vulkan state:
MoltenVK 1.4.1's Metal resource/content lifetime or render-encoder/state reset.

## Target and change set

- Keep the selected ESO 12.0.7 fingerprint and MoltenVK 1.4.1 runtime.
- Keep both exact HDR filters, live-resource checking, disabled Metal argument
  buffers, MTLHeap `where safe`, synchronous submission, command pooling, and
  command-buffer prefill disabled.
- Remove Experiment 0015's reset-only pipeline-cache bypass. Pipeline creation
  is forwarded unchanged in every phase.
- Select a distinct `full-lifetime-audit` marker mode.
- Enable descriptor mirroring at process sequence 1, before any swapchain.
- Mirror descriptor-set layouts, immutable samplers, binding/array slots,
  image views, samplers, buffers, and buffer views.
- Trace command-buffer allocation, free, explicit reset, pool reset, begin,
  end, descriptor record order, generation, and queue submission.
- Record attachment load/store/stencil operations, clear-value presence,
  attachment roles, and first/last subpass use.
- Expand image transitions by aspect, mip, and layer. Retain stage/access masks
  at record time and apply transitions in queue-submit order.
- Trace acquire, submit, present semaphore order and fence create/reset/wait
  relationships.

The implementation uses fixed-size tables only. Ordinary hot paths do not
write per-call files. The eight-presentation reset window emits bounded
attachment, descriptor-slot, barrier, and command-submit samples plus aggregate
counters.

## Preflight

- Verify all Experiment 0015 checksums before the technical restore.
- Run `scripts/check-update.sh` and require the selected target.
- Require ESO, Steam, and the launcher to be stopped.
- Confirm the active loader, pristine restore source, marker, both pipeline
  caches, and exact target fingerprint.
- Commit the source candidate before building so evidence names one immutable
  revision.
- Perform a cache-preserving restore, rebuild from source, and run the full
  non-game gate.
- Require zero fixed-table overflow in probes and reject incomplete analyzer
  coverage.
- Install only the exact `full-lifetime-audit` mode and compare built and
  installed hashes.
- Prepare ignored evidence before requesting a user launch.

The user explicitly approved installation of this next candidate in the task
request, subject to those fail-closed checks.

## Non-game validation

Focused probes already establish:

- the descriptor mirror begins at sequence 1;
- full layout/binding slot coverage includes image, buffer, and immutable
  sampler contents;
- destroyed descriptor resources become stale;
- explicit command-buffer reset and command-pool reset both advance valid
  generations before later submission;
- an acquire semaphore is consumed by submit, its signal semaphore is consumed
  by present, and the chain has no unknown wait;
- a barrier expands to its exact subresource and retains source/destination
  stage and access masks;
- attachment layout transitions are applied in command submission order;
- an inactive draw wrapper remains below the existing one-microsecond smoke
  threshold, and a mirrored descriptor update remains below the existing
  ten-microsecond threshold.

Source commit `7db8803` passes 68 Python tests, Python compilation, shell
syntax, whitespace checks, warnings-as-errors, and Clang static analysis. The
complete pristine-loader build passes Bink re-export, Rosetta self-patch, HDR,
lifecycle, reset-resource, full render-audit, and all eight effective
configuration probes.

Fresh replacement and embedded-runtime probes reached the Apple M4. They
reproduced the established HDR negotiation and 100-name proc-address results;
the surface probes reproduced 60 MoltenVK 1.4.1 formats with the exact ESO HDR
pair versus three embedded-runtime formats without it.

## Procedure

After installation and evidence preparation, the user will:

1. launch once through Steam and enter the existing world;
2. change fullscreen resolution once from 1920 x 1200 to 2048 x 1280;
3. wait only long enough to classify normal rendering, solid color, frozen
   last frame, or crash, then exit normally.

No second graphics option, travel, HUD, capture, FPS record, or extended play
is required. If this one launch exhibits the known low-performance state, no
immediate repeat will be requested; its full-lifetime evidence will be
collected and analyzed first.

## Pass/fail

- **Diagnostic pass:** the reset window completes with process-sequence-1
  mirroring, known descriptor/layout coverage, valid command generations, and
  no table overflow. The captured signal identifies one remaining category, or
  cleanly leaves only the internal Metal/MoltenVK category.
- **Rendering pass:** output remains correct after the one reset.
- **Fail:** solid color, frozen output, or crash recurs with complete evidence.
- **Inconclusive:** any table overflows, a required wrapper or summary is
  missing, the target changes, no reset window completes, or coverage contains
  an unknown state that prevents classification.

## Evidence

Before restoration, all 48 Experiment 0015 evidence files passed their
`SHA256SUMS` manifest. The selected ESO SHA-256, UUID, client version, and
databuild remained current. ESO, Steam, and the launcher were stopped. The
active loader was the current Experiment 0015 bridge, its marker was present,
and both pipeline caches matched their preserved hashes.

A cache-preserving restore returned the active loader to the checked pristine
source and displaced the old marker. Post-restore status reported the exact
target, original/inactive loader, and absent marker; both cache hashes remained
unchanged.

The complete source build and non-game gate above then passed. The candidate
was installed in exact `full-lifetime-audit` mode under the task's explicit
approval. Built and installed files are byte-identical:

```text
libBink2Macx64.dylib
0676aa70ada8623b726de0a4613c406a600547d3171337954dba8f9379bbbdcb

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

Post-install status reports the bridge target current and marker present; the
marker contains `full-lifetime-audit` and the quick gate is `READY`. The active
4,259,071-byte cache and 6,800,792-byte old backup retain their pre-restore
hashes. The authoritative ignored collection directory is
`artifacts/experiment-0016-20260725T135200Z`; the earlier
`artifacts/experiment-0016-20260725T135008Z` preparation is preserved but is
superseded because it predates this final documentation commit. No agent
launched Steam, the launcher, or ESO.

## Result

Pending.

## Interpretation

Pending.

## Rollback

Not performed. The checked pristine loader and both unchanged pipeline-cache
files remain the restore boundary.

## Follow-up

If one public category is implicated, implement only a distinguishable narrow
counterfactual against that category. If all public categories are clean,
instrument MoltenVK's internal Metal texture/resource or render-encoder reset
boundary rather than adding another configuration A/B.
