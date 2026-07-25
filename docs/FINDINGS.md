# Findings

This document contains durable observations promoted from completed work. Run
specific procedures, failures, and hypotheses remain in the
[experiment records](experiments/README.md).

## Test platform

- Apple M4 MacBook Air
- Steam macOS ESO client, x86_64 under Rosetta
- Rendering path reported by Metal HUD: Metal, Direct
- Captured backing resolution: 3420 x 2146
- ESO executable SHA-256 during analysis:
  `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`

## Repeatable frame-rate degradation

The same general scene and workload can run near 54-56 FPS, then fall to about
33 FPS after continued play. Logging out of the world and logging back in
restores about 56 FPS without restarting the launcher.

Metal HUD observations:

| State | FPS | GPU time | App memory | Metal memory | Thermal |
|---|---:|---:|---:|---:|---|
| Healthy capture A | 55.81 | 14.48 ms | 4.78 GB | 1.39 GB | Nominal |
| Healthy capture B | 53.73 | 14.85 ms | 4.74 GB | 1.37 GB | Nominal |
| Degraded capture | 33.80 | 25.72 ms | 5.82 GB | 1.46 GB | Nominal |

Interpretation:

- Thermal throttling was not active in these captures.
- The degraded state is GPU-bound according to the HUD.
- App memory increased by roughly 1 GB while Metal memory rose modestly.
- Recovery after leaving the world suggests retained per-zone, per-character,
  render-pass, descriptor, or command state. This is evidence of accumulation,
  not proof of one specific leak.

## Metal performance warning

The HUD repeatedly reports a high number of render passes with similar
attachments and recommends merging passes or using color attachment mapping.
This is an engine/render-graph warning. A user setting cannot safely merge
logical render passes, and a newer MoltenVK cannot assume that two Vulkan render
passes are semantically mergeable.

## Runtime architecture

- Bundled headers identify MoltenVK 1.0.18.
- The bundled `MoltenVK.framework` payload is an `ar` static archive, not a
  dynamically loaded framework.
- ESO has no MoltenVK or Vulkan dynamic dependency.
- MoltenVK classes and Vulkan wrappers are linked into the ESO executable.
- Therefore swapping the framework/archive does nothing to an already linked
  executable.

## Why MoltenVK 1.4.1 is still worth investigating

Official 1.4.1 testing on the M4 confirmed support for ESO's old instance and
device extension set. The release also contains a new descriptor state tracker,
new descriptor set/pool implementation, and occlusion-query improvements. These
areas overlap with the observed accumulation pattern, but no performance gain
has been demonstrated in ESO because the first full bridge test crashed.

## Public Vulkan routing coverage

For the fingerprinted ESO executable, expanded Mach-O analysis found 40
external references to 17 old MoltenVK entry points. The redirect manifest
covers all 17, and MoltenVK 1.4.1 exports each one. Static analysis also
recovered every direct proc lookup: 17 unique GIPA names and 65 unique GDPA
names.

The initial old/new 100-name probe found no old-nonnull/new-null change, but it
tested only one device-extension configuration. Experiment 0002 showed why that
qualification matters: the last recorded startup lookup was a NULL GDPA result
for `vkSetHdrMetadataEXT`, followed by the same `RIP=0` and first recoverable
ESO return offset observed in Experiment 0001. Experiment 0004 reproduced the
same state after successfully hiding the HDR device extension.

MoltenVK 1.0.18 does not advertise `VK_EXT_hdr_metadata` and returns NULL for
that proc. MoltenVK 1.4.1 advertises the extension; GIPA returns non-null, while
GDPA returns NULL until the extension is enabled on the device and non-null
after it is enabled. Proc compatibility tests must therefore reproduce both the
advertised extension set and the extensions enabled at device creation.

The NULL call is now confirmed. In both Experiments 0002 and 0004, Rosetta's
saved `tmp1` equals the ASLR-adjusted ESO instruction immediately after the
indirect `vkSetHdrMetadataEXT` call. The ordinary report frames begin at its
outer virtual caller, which is why the earlier report alone appeared less
specific.

## HDR compatibility and surface-format trigger

The shared compatibility implementation passes both fake-runtime semantics
tests and a real MoltenVK 1.4.1 non-game probe. The real probe observes HDR in
the raw device-extension list, hides it from the visible list, leaves it
disabled at device creation, obtains a non-null GIPA result and NULL GDPA
result for `vkSetHdrMetadataEXT`, and creates and destroys the device
successfully.

Experiment 0004 confirmed all of those transitions inside ESO but still reached
the setter query and NULL call. Static disassembly establishes that ESO's
setter path is instead guarded by an internal flag set when
`vkGetPhysicalDeviceSurfaceFormatsKHR` exposes
`VK_FORMAT_A2B10G10R10_UNORM_PACK32` with
`VK_COLOR_SPACE_HDR10_ST2084_EXT`. Hiding the device extension does not alter
that independent list.

On the same M4, a non-game AppKit/Metal probe returned three sRGB surface
formats from embedded MoltenVK 1.0.18 and no matching HDR pair. MoltenVK 1.4.1
returned 60 formats and one exact matching pair. A source-only wrapper that
removes only that pair returned 59 formats and passed count/data validation.

Fake-runtime coverage now verifies count-only, exact-capacity, short-capacity
`VK_INCOMPLETE`, property order, exact pair matching, preservation of other HDR
formats, layer-specific device-extension pass-through, and unchanged
device-create forwarding. These findings establish the local wrapper semantics
and the failed-run root cause.

## Startup compatibility passed, rendering correctness did not

In one controlled Experiment 0005 run, the exact surface-format wrapper was
active for both count and data queries and removed one pair from 60 raw to 59
visible formats. ESO never queried `vkSetHdrMetadataEXT`, continued through
swapchain and pipeline setup, remained alive at character selection for more
than 60 seconds, and exited through normal AppKit termination. The automatic
bridge verdict passed and no crash report was created. This confirms the HDR
surface pair as the trigger for the repeated startup crash on the fingerprinted
build.

The same run was not rendering-correct. The user observed a full-screen hot-pink
frame during startup and high-frequency flicker of a black, shadow-like layer at
character selection despite otherwise smooth animation. This blocked gameplay
and performance testing in Experiment 0005.

The unified log recorded 106 Metal compiler warnings during shader and texture
loading, but macOS redacted their contents. They are correlated evidence only,
not proof that compilation caused either visual symptom. ESO's own logs reported
renderer and texture completion without a relevant error. The new 3,141,826-byte
pipeline cache and prior 6,800,792-byte cache are both preserved with hashes in
the ignored evidence.

## Metal argument-buffer compatibility boundary

Experiment 0006 retained the same MoltenVK 1.4.1 runtime, live-resource check,
HDR filters, and 17 redirects, and disabled only Metal argument buffers. The
runtime reported the effective change before activation. In that run the
Experiment 0005 black/shadow-layer flicker was absent at character selection
and through world entry, and world rendering remained visually correct for
about 2 minutes 33 seconds before the user changed a graphics setting.

This single-variable A/B strongly implicates the Metal argument-buffer
descriptor path in the Experiment 0005 corruption. It establishes a first
short gameplay-capable baseline with ambient occlusion disabled, not extended
stability or support for every graphics option.

The transient hot-pink frame remained in Experiment 0006 and again during the
maintenance-interrupted Experiment 0007 launch, both before the game UI. It
therefore does not track the argument-buffer change or the 12.0.7 target rebase.
Because Experiment 0006 subsequently rendered character selection and the
world correctly, the pre-UI frame alone is not evidence of persistent world
corruption. Its cause remains unresolved.

The 12.0.7 world repeat added a second short rendering-correct interval. ESO's
own interface log reached character selection and fully loaded Auridon; the
user explored for approximately 162 seconds without perceptible stuttering or
visual corruption. The bridge verdict passed and no crash occurred. This
supports short gameplay stability with the rewritten cache, but is not the
planned five-minute interval or a controlled performance comparison.

Experiment 0008 set `SkipPregameVideos` to `1`. ESO's interface log then began
directly at `AccountLogin` with no `PlayIntroMovies`, ZOS video, Havok, or legal
splash state, proving that the setting bypassed the logged video sequence. The
user still observed the same transient hot-pink frame. The artifact therefore
occurs outside those logged video states, likely before or during initial
swapchain/UI presentation; it remains non-fatal in the tested configuration.

The later live switch to SSAO coincided with ESO's graphics-device reset and
reduced output to changing solid colors without a Vulkan error, device loss, or
process crash. Experiment 0009 independently reached the same solid-color class
after lowering fullscreen resolution from 2048 x 1280 to 1920 x 1200. Both
events crossed `DeviceWaitIdle`, swapchain recreation, and `OnDeviceReset`
after the world had loaded. The resolution-only trigger strengthens the common
live reset/resource-recreation path over an SSAO-specific shader explanation,
but does not identify the incorrect resource owner or lifecycle operation.

Experiment 0011 changed only MoltenVK's effective MTLHeap mode from `where
safe` to `never`. The ESO process verified MTLHeap `0`, then reproduced
persistent solid-color output after changing fullscreen resolution from
1920 x 1200 to 2048 x 1280. Its third swapchain generation acquired and
presented 382 frames with clean tracked swapchain-dependent lifetimes, no
device loss, and no process crash. MTLHeap allocation is therefore not the
cause of the observed live-reset corruption and should not be disabled as a
workaround for it.

Experiment 0012 proved that the corrupt post-reset output is still backed by
substantial submitted rendering work. Its first eight replacement-swapchain
frames contained eight submitted command buffers, 484 balanced render passes,
7,699 indexed draws, 7,699 descriptor-set binds, and no recorded Vulkan
failure. The same bounded interval created 65 images, 67 image views, 53
buffers, and 119 graphics pipelines successfully. This rules out command
starvation and a general resource-creation failure.

Those new images and buffers reused existing `VkDeviceMemory`; no allocation,
free, map, or unmap occurred in the bounded window. The 15 complete image
bindings captured by the detail cap were aligned and non-overlapping. This
rules out simple offset misalignment and overlap among that captured subset,
not all old-versus-new aliases.

The reset also destroyed 77 image views, created 67, allocated 456 descriptor
sets, and made 93,707 `vkUpdateDescriptorSets` calls in eight frames.
MoltenVK 1.4.1's official source and release notes identify a new descriptor
state/set implementation and show that the enabled live-resource compatibility
mode skips non-live descriptor targets at Metal binding time. Embedded
MoltenVK 1.0.18 instead stores Metal resource objects when descriptors are
updated and has no equivalent tracker. This independently supported version
delta and the local reset counts make descriptor/resource state the leading
region, but do not yet identify a specific invalid descriptor.

Experiment 0013 changed only MoltenVK internal command pooling from enabled to
disabled and verified the effective value inside ESO. Persistent solid-color
output still followed a 1920 x 1200 to 2048 x 1280 reset. The replacement
swapchain then acquired and presented 262 frames; its first eight contained
9,671 indexed draws and descriptor-set binds with no Vulkan failure.
MoltenVK's internal command-memory pool is therefore not the primary cause.
This result does not exclude ESO's reuse of already allocated Vulkan command
buffers or descriptor sets.

Experiment 0014 joined descriptor contents, image-view lifetimes, image-memory
ranges, barriers, render-pass attachments, and graphics pipelines in the same
bounded reset window. Three completed audits had zero mirror overflow, unknown
handle, stale descriptor-set bind, live image overlap, tracked layout mismatch,
dead attachment view, or pipeline/render-pass mismatch. Hundreds to thousands
of descriptor slots still named a view when that view was destroyed, but none
of the affected sets was later bound. This excludes the simple
destroyed-view-rebound explanation, not every descriptor ordering semantic.

The same candidate exposed two user-visible states: an initial 8 FPS phase in
which graphics resets remained usable, followed after complete Steam-path
restart by a 60 FPS phase in which a resolution reset again produced solid
color. Every reset-created graphics pipeline in all completed audits received
the active pipeline cache and was later bound to the exact matching render
pass. The active cache grew by 55,314 bytes over the multi-run session. This
behavioral split makes reset-only pipeline-cache bypass the next narrow
counterfactual; it does not yet prove cached pipeline corruption.

Experiment 0015 completed that counterfactual. All 150 reset-created graphics
pipelines were forwarded with `VK_NULL_HANDLE` instead of ESO's cache, zero
reset pipeline used a cache, and all 1,648 observed binds of those pipelines
matched the active render pass. Persistent solid-color output still followed
the live resolution change. The pipeline cache is therefore excluded as the
cause of the reset-created pipelines' rendering corruption.

The combined audit has one important scope limit. Descriptor mirroring begins
at the first successful swapchain, and many sampled final-pass descriptor sets
at set index 0 have no mirrored update (`last_update_sequence=0`). Their
handles are known and live, but their slot contents may have been established
before mirroring began. Consequently the zero stale-set result excludes a
rebound of a *known* destroyed view; it does not yet exclude a persistent
descriptor set whose contents predate the first swapchain.

## MoltenVK 1.4.1 swapchain image-view cache defect and ESO boundary

A focused non-game M4 probe now reproduces an internal MoltenVK 1.4.1 failure
that the public Vulkan audits could not observe. The probe renders a changing
clear color through a component-swizzled image view, then reads the current
`CAMetalDrawable` base texture. When the same swapchain `VkImage` acquired a
new base `MTLTexture`, official 1.4.1 left that texture untouched while its
cached derived image view continued to receive rendering on the earlier
drawable. Acquire, command submission, and presentation all remained valid.

The exact v1.4.1 source with only upstream commit `9a5e233` backported passed
the same drawable-replacement probe and wrote every expected current-frame
pixel. The upstream change invalidates and recreates a cached image-view Metal
texture when its base, parent/root texture, or IOSurface identity changes. This
establishes the mechanism and the narrow repair independently of ESO.

Experiment 0017 established the missing applicability boundary. Every captured
ESO swapchain view used identity component mappings, the swapchain format, and
the complete remaining color subresource range. MoltenVK therefore uses the
presentable image's current base texture directly and does not create the
cached derived texture changed by `9a5e233`. An added identity-view probe
confirms that official 1.4.1 and the backport both write the replacement
drawable correctly on that path.

The user run then entered the world at restored performance and blacked out
after one 2048 x 1280 to 1920 x 1200 reset. Cursor, input, and sound remained
responsive, no Vulkan or lifecycle failure appeared, and the replacement
swapchain continued for 797 acquire/present pairs. The backport is thus a valid
upstream repair but not the repair for ESO's reset corruption. The restored
performance separately supports Experiment 0016's audit implementation as the
source of its severe low-FPS state.

## Core feature enablement differs from the embedded runtime

Complete Apple M4 device-profile probes show that embedded MoltenVK 1.0.18
reports 18 enabled Vulkan 1.0 core features while official 1.4.1 reports 36.
The 18 additions include robust buffer access, tessellation, sample-rate
shading, multi-viewport, dynamic descriptor indexing, storage-image
without-format access, 64-bit shader integers, resource minimum LOD, and
inherited queries.

Static analysis of ESO 12.0.7 makes this difference operational rather than
merely advertised. ESO first requires ten features that are true in both
runtimes. After selecting the device, it queries the complete feature
structure again and passes that structure unchanged as
`VkDeviceCreateInfo.pEnabledFeatures`. The bridge therefore creates a device
with all 36 replacement-runtime features enabled, whereas the embedded path
created one with 18.

An automated non-game M4 counterfactual clears only the 18 additions. Every
one of the resulting 55 feature values exactly matches embedded 1.0.18, and
real MoltenVK 1.4.1 accepts ESO's extension set and creates the device. This
does not yet prove which added feature causes the loaded-world reset failure;
it establishes the exact, narrow compatibility boundary used by Experiment
0018.

MoltenVK 1.4.2 does not change this core profile. With Metal argument buffers
disabled, its core features, limits, and sparse properties are identical to
1.4.1; only API/driver version and pipeline-cache UUID differ. Both versions
also pass a 24-cycle non-game reset composite using a preallocated descriptor
set, exact alternating source extents, recreated render resources, and
alternating command-buffer/pool resets. A wholesale 1.4.2 upgrade therefore
has no demonstrated reset-relevant differential at this checkpoint.

Experiment 0018 then exposed exactly the embedded 18-feature profile to ESO
and validated that device creation enabled those 18 with no prohibited field.
One loaded-world resolution reset still produced solid-color output while 313
acquire/present pairs continued. The added core-feature category is therefore
excluded.

That run also shows that the supposedly bounded lifecycle trace is not bounded
after ESO's reset. All 313 replacement-generation acquires and presents
returned `VK_SUBOPTIMAL_KHR`. The wrapper logs every non-success result, so it
performed per-frame mutex, formatting, and file flush work for 626 records.
This is a confirmed performance-path defect in the diagnostic bridge, although
it is not yet established as the cause of rendering corruption.
