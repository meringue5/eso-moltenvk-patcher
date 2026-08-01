# Experiment 0026: bounded startup pre-present pixel audit

- Date: 2026-08-01
- Outcome: **prepared; non-game discrimination passes, installation awaits explicit approval**
- Rollback: **not applicable; the installed `performance-aggressive` bridge, caches, and settings remain unchanged**

## Question

Does canonical magenta already exist in ESO's final swapchain image immediately
before `vkQueuePresentKHR`, or is the normally colored image changed later by
MoltenVK presentation, `CAMetalLayer`, CoreAnimation, or window composition?

## Why this is the next gate

Experiment 0024 excluded a submitted Vulkan clear: one generation-1 and 180
generation-2 full-surface clears were all opaque black while the user saw the
same pink frame. Experiment 0025 could not exercise its exact FX-material
initializer hook and is therefore inconclusive. Following another material
constructor or shader name would still leave the final pixel association
unproven.

The swapchain image is the narrow boundary shared by every application draw.
Reading it immediately before presentation divides the remaining source space
without assuming which ESO material, shader, video, or fallback produced it.

## Candidate

The isolated `startup-present-pixel-audit` mode retains the exact effective
official MoltenVK 1.4.2 `performance-aggressive` configuration and the existing
two-generation ordinal-180 bound. It adds no render or color mutation.

For generation 1 ordinal 1 and generation 2 ordinals 1, 30, 60, 90, 120, 150,
and 180, the lifecycle wrapper:

1. resolves the exact swapchain image by swapchain and image index;
2. proves every present wait semaphore was most recently signaled by a submit
   on the same queue, consuming that proof after presentation;
3. calls the real `vkQueueWaitIdle` only for the scheduled frame;
4. obtains the image's `MTLTexture` through `vkGetMTLTextureMVK`;
5. blits the center and four quarter points into shared memory before the real
   present;
6. logs raw bytes, normalized RGBA, exact `255,0,255`, near-magenta, and black
   counts.

The current ESO swapchain is `VK_FORMAT_B8G8R8A8_UNORM` (`44`), so the sampler
explicitly converts BGRA storage to RGBA. Unsupported formats, missing image
mapping, unconfirmed synchronization, failed queue waits, Metal errors, a
missing sample, or a missing finish all fail closed as `INCONCLUSIVE`.

The eight queue-idle points can perturb startup timing, so this mode is a
one-run diagnostic and is never enabled by normal `performance-aggressive`.
It does not alter caches, settings, render commands, or pixel contents.

## Non-game validation

No Steam, launcher, or ESO process was launched. The installed game bundle,
active and backup pipeline caches, and settings were not changed.

The real MoltenVK/AppKit startup-surface probe exercises the same sampler and
lifecycle callback at both 64-pixel control extents and the exact ESO
3420 x 2148 to 3420 x 2146 transition. It records:

```text
canonical-magenta control: raw BGRA 255,0,255,255; RGBA 255,0,255,255
five-point magenta summary: exact_magenta=5 near_magenta=5 black=0
five-point black summary: exact_magenta=0 near_magenta=0 black=5
Startup surface non-game probe: PASS
108 Python tests: PASS
Lifecycle trace smoke: PASS
Reset resource trace smoke: PASS
temporary full bridge link and BinkOpen re-export: PASS
startup-present-pixel-audit MoltenVK configuration probe: PASS
python compileall, shell syntax, and git diff check: PASS
```

The ordinary `scripts/build.sh` correctly refused to reuse an installed bridge
as its source. A complete bridge was therefore linked only under `/tmp` from
the preserved renamed original for compile/link validation; no installed file
was replaced. The normal clean restore, source build, and install sequence is
deferred to the explicit installation gate.

## One-run decision table

`tools/analyze_startup_present_pixels.py` selects the newest exact-mode run and
requires all eight summaries, five points each, exact format 44, no skip/error,
and the ordinal-180 finish.

| User sees pink in the exact run | Pre-present sample contains magenta | Verdict |
|---|---:|---|
| yes | yes | `SWAPCHAIN-MAGENTA-CONFIRMED`: the color already exists in final ESO/MoltenVK image content |
| yes | no, with all eight samples complete | `POST-SWAPCHAIN-MAGENTA`: prioritize presentation/layer/compositor processing |
| no | yes | `SWAPCHAIN-MAGENTA-NOT-DISPLAYED`: image content and visible result diverged |
| no | no | `NO-PINK-CONTROL`: the recurring symptom was not exercised in this run |
| either | missing, skipped, failed, or unbounded | `INCONCLUSIVE` |

## Installation gate

The system refused the cache-preserving restore because approvals for prior
experimental modes do not authorize this new mode. The refusal occurred before
any filesystem change. The current marker remains `performance-aggressive`,
and the active cache, old-backup cache, and settings hashes remain:

```text
active cache: aed8bce13b26a8d2760b69d34440d27b1bdf244b4cdee6490bd847f759b904ba
old-backup cache: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
settings: 297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c
```

Installation requires one explicit approval naming
`startup-present-pixel-audit`. After approval, verify Steam, the launcher, and
ESO are closed; perform a cache-preserving restore; rebuild from source;
install the exact mode with cache preservation; verify installed/build bytes
and all three hashes; then stop for one user-controlled Steam-path startup.
