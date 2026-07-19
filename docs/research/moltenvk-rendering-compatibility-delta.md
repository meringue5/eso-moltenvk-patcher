# MoltenVK rendering compatibility delta after Experiment 0005

- Review date: 2026-07-19
- Scope: official MoltenVK tags, documentation, issues, and adjacent public
  rendering reports
- Question: Which single runtime difference should be tested first after the
  Experiment 0005 startup pass exposed severe visual corruption?

## Local boundary

Experiment 0005 established locally that the exact HDR surface-format filter
prevents ESO's NULL setter crash. It did not establish general rendering
compatibility. The hot-pink startup frame and black/shadow-layer flicker are
user observations; the bridge and system logs identify no failed Vulkan call.

The unified log's 106 Metal compiler warnings are privacy-redacted. Their timing
overlaps renderer and texture loading, but no public text is available to match
against an upstream issue. They are not treated as a root cause.

## Upstream configuration delta

The official [MoltenVK 1.0.18 configuration header](https://github.com/KhronosGroup/MoltenVK/blob/v1.0.18/MoltenVK/MoltenVK/API/vk_mvk_moltenvk.h)
contains no Metal argument-buffer, MTLHeap, command-pooling, or live-resource
configuration. Its queue-submission default is asynchronous.

The official [MoltenVK 1.4.1 configuration reference](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/MoltenVK_Configuration_Parameters.md)
documents these relevant defaults:

| Parameter | 1.0.18 equivalent | 1.4.1 default |
|---|---|---:|
| Metal argument buffers | feature absent | enabled |
| MTLHeap | feature absent | where safe / active on Apple GPU |
| live-check all resources | feature absent | disabled; explicitly enabled by `teso4m4` |
| synchronous queue submits | disabled | enabled |
| command pooling | no public control | enabled |
| command-buffer prefill | absent | disabled |

The [1.4.1 change history](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/Whats_New.md)
also records a new descriptor state tracker and descriptor set/pool
implementation, with `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1` as the compatibility
escape hatch for old applications that retain destroyed descriptor resources.

## Public adjacent cases

- [MoltenVK issue #2278](https://github.com/KhronosGroup/MoltenVK/issues/2278)
  reports an argument-buffer/descriptor-indexing GPU address fault whose behavior
  changes with descriptor layout inputs. ESO did not enable descriptor indexing,
  so this is mechanism precedent rather than a symptom match.
- [MoltenVK issue #2530](https://github.com/KhronosGroup/MoltenVK/issues/2530)
  reports application-dependent behavior when Metal argument buffers are
  toggled for older graphics workloads. It concerns performance, not ESO's
  visual corruption, but confirms that the default is not neutral for every
  legacy application.
- An [mpv MoltenVK discussion](https://github.com/mpv-player/mpv/discussions/14138)
  mentions flicker and black screens around drawable-size handling. That is a
  different known mechanism and does not currently justify changing ESO's
  surface dimensions or swapchain path.

No public ESO report or exact match for the observed high-frequency
shadow-layer flicker was found in the reviewed material.

## Decision

Disable Metal argument buffers first and change nothing else. This choice is
narrow because the feature did not exist in 1.0.18, is enabled by default in
1.4.1, changes descriptor resource binding directly, and is unnecessary for
the three device extensions ESO enabled in the captured run.

This is a diagnostic A/B, not a claimed fix. If it does not improve either
predefined visual symptom, restore the variable and test MTLHeap and legacy
asynchronous submission separately. Do not combine those controls, because a
multi-variable result would not identify the compatibility boundary.
