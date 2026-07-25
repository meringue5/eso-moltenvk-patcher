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
