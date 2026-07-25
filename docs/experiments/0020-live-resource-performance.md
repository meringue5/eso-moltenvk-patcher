# Experiment 0020: live-resource descriptor performance

- Date: 2026-07-26
- Outcome: **prepared; measured non-game gain, installation not started**
- Rollback: **Experiment 0019 remains installed; pristine loader checked**

## Question

Does disabling MoltenVK 1.4.1's all-resource liveness checking remove measurable
CPU work from ESO's argument-buffer-disabled descriptor path without breaking
valid descriptor reuse in the existing non-game reset coverage?

## Scope

This is a performance experiment, not another reset-repair attempt. Experiment
0019 remains the installed checkpoint. The only additional runtime change in
the new `performance-aggressive` mode is:

```text
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=0
```

The complete profile is:

```text
MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=0
MVK_CONFIG_USE_MTLHEAP=1
MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0
MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1
MVK_CONFIG_USE_COMMAND_POOLING=1
MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0
```

It retains official MoltenVK 1.4.1, the 17 checked redirects, both HDR filters,
the required device/proc compatibility routes, and direct routing for all
twelve former lifecycle wrappers.

## Static mechanism

In MoltenVK 1.4.1, with Metal argument buffers disabled,
`MVKPipelineLayout::populateBindOperations()` selects
`BindTextureWithLiveCheck`, `BindBufferWithLiveCheck`, and
`BindSamplerWithLiveCheck` for every descriptor binding when
`liveCheckAllResources` is enabled.

When a bound resource changes, those operations call
`MVKLiveList::isLive()`. The list hashes the Objective-C resource into one of
16 groups, acquires an `os_unfair_lock`, and searches the group's live-resource
map. Resource creation and destruction also update those locked maps. This is
real descriptor-encode work, not merely validation logging.

The option also requests retained Metal command-buffer references when no
residency set is present. In this profile argument buffers are disabled, so
MoltenVK does not create its argument-buffer residency set.

## Non-game performance measurement

The M4 probe uses official x86_64 MoltenVK 1.4.1 through Rosetta. It records
20,000 draws into a one-pixel target while alternating two valid combined
image-sampler descriptor sets backed by different textures. Pipeline creation,
resource creation, command recording, and one warm-up submission are outside
the timed region.

Synchronous submission is used only by the measurement probe so that
MoltenVK's CPU command encoding remains inside the measured `vkQueueSubmit`
interval. GPU/fence wait time is measured separately. Three balanced processes
per setting provide 21 post-warm-up samples each.

```text
live check on:
  aggregate median submit = 3,916,667 ns
  median per draw          = 195.833 ns
  process medians          = 208.579, 192.435, 185.083 ns/draw

live check off:
  aggregate median submit = 3,521,792 ns
  median per draw          = 176.090 ns
  process medians          = 174.823, 180.392, 176.090 ns/draw

measured submit-time reduction = 10.082%
classification                 = MEASURED_GAIN
```

Every sample produced the expected final Metal pixel. This result measures a
10.1% reduction in this deliberately descriptor-heavy CPU encoding interval.
It does not imply a 10.1% ESO FPS increase or a GPU-time reduction.

Raw measurement output is preserved outside the repository at
`/private/tmp/teso4m4-descriptor-performance-0020`.

## Correctness boundary

With the exact aggressive profile, official 1.4.1 passes all 24 reset-composite
cycles. The probe alternates 2048 x 1280 and 1920 x 1200 resources, updates one
full-lifetime descriptor set, alternates command-buffer and command-pool reset,
submits asynchronously, and verifies the expected Metal pixel every cycle.
The comparison runtime also passes all 24 cycles.

The profile also passes real M4 ESO-era device creation, the 100-function proc
profile, and the 60-to-59 HDR surface-format filter.

This does not prove that every ESO descriptor is valid. The prior full-lifetime
audit found no observed descriptor rebinding a known destroyed image view, but
descriptor contents established before its first swapchain remained unknown.
If ESO relies on MoltenVK skipping a destroyed resource, disabling the check can
turn a skipped binding into corruption or a crash. The candidate is therefore
named `performance-aggressive`, and startup must fail closed unless the exact
configuration is verified.

## Source and build validation

- Clean shadow-bundle source build passes Bink re-export and Rosetta self-patch.
- All twelve MoltenVK configuration modes pass, including exact aggressive
  configuration verification.
- Existing HDR, feature-profile, lifecycle, reset, and render-audit smoke
  probes pass.
- All 80 Python tests, Python compilation, shell syntax, whitespace checks,
  warnings-as-errors builds, and Clang static analysis pass.
- Prepared artifact SHA-256:

```text
source:    ed5b9d3 (Prepare Experiment 0020 descriptor performance)
proxy:     5019d4eb552f89ea59bfda9d38e2f2c98ce36f2490c7c41294750c62ba68acde
MoltenVK:  d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

No Steam, launcher, or ESO process was started for Experiment 0020.

## Installation gate

- Recheck the target fingerprint and stopped processes.
- Verify the installed Experiment 0019 files, pristine loader, settings, both
  pipeline caches, and prepared hashes.
- Receive explicit approval for this game-bundle modification.
- Preserve both caches and settings, restore the pristine loader, rebuild from
  that real loader, and require byte-identical artifacts.
- Install only `performance-aggressive` and verify its exact marker and
  configuration boundary before any user launch.

## Result

The source candidate has a repeatable non-game descriptor-encode gain and
passes the available valid-resource correctness coverage. Installation has not
started.

## Rollback

Experiment 0019 remains installed. The pristine loader, both pipeline caches,
settings, and all prior evidence remain preserved.
