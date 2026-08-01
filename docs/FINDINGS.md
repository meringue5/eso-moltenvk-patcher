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

The user reports that this was the long-standing embedded MoltenVK 1.0.18
experience on the M4: medium settings were not practical, minimum-oriented
settings still struggled to reach 50 FPS, object-heavy areas repeatedly fell
toward 30 FPS, and leaving or reloading the current UI/world state was used as
a recovery workaround. Graphics-option changes could also destabilize or crash
the vanilla client. These are direct user observations; the captures below
provide the independently measured subset of that history.

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

That reset-repair conclusion does not imply that remaining on 1.4.1 is the
better maintenance choice. A later review of the official 1.4.2 release
verified its archive and universal runtime, exact Experiment 0020
configuration, complete bridge build, device/proc compatibility, HDR filters,
and the same 24-cycle reset coverage. Its render-pass subpass-dependency
barrier correction is below the prior public-state audit and cannot be
excluded from ESO's full render graph, although it is not proven to repair the
symptom.

In a balanced descriptor-heavy comparison, 1.4.2 was 2.293% slower than 1.4.1,
below the established 3% meaningful-change threshold with overlapping process
medians. The evidence supports 1.4.2 as a maintenance candidate without a
material measured descriptor-path regression, not as a demonstrated FPS
improvement or graphics-reset fix. Its different pipeline-cache UUID requires
a separate cold cache and preservation of the 1.4.1 cache for rollback.

Experiment 0020 subsequently passed an ordinary ESO session with its exact
`performance-aggressive` profile. Startup verification passed, the interface
reached its loaded-world state, no crash report was produced, and the user
reported no problem. This supports the profile as the current ordinary-use
performance checkpoint. It remains insufficient to quantify an ESO FPS gain
or to reverse the prior loaded-world graphics-reset failures.

Official MoltenVK 1.4.2 was then installed with that profile after its source
build and complete non-game gates passed. Its separate cache identity is
enforced operationally: the exact 1.4.1 runtime and `db445ff2` cache are
preserved under versioned names, the older cache remains untouched, and 1.4.2
starts without an active cache. This establishes a reversible maintenance
baseline, not a demonstrated repair for the graphics-reset corruption.

## The current 1.4.2 checkpoint no longer reproduces reset corruption

Post-install ordinary play materially changes the current operational result
without invalidating the earlier 1.4.1 failures. The latest preserved 1.4.2 run
passed bridge startup, logged six complete graphics-device reset sequences with
zero error markers, repeatedly completed world loading, and produced no crash
report. The user directly observed normal scene rendering after resolution and
graphics-setting changes. With the committed 2048 x 1280 standard settings,
the user also observed the on-screen counter remain at the 60 FPS VSync ceiling
throughout active gameplay in the roughly 93-minute session. No comparable
sustained frame-rate degradation occurred.

The ESO, interface, and bridge logs do not contain continuous FPS samples, so
the 60 FPS statement is a direct user observation rather than automated
telemetry. The logs independently establish the session boundary, exact bridge
configuration, repeated world loads, six reset sequences, and absence of a
subsequent crash report.

This establishes successful reset behavior for the combined official 1.4.2,
`performance-aggressive`, and 1.4.2-cache checkpoint. The evidence does not
attribute the performance or correctness improvement to a particular 1.4.2
fix, nor does it prove that the same behavior generalizes to another ESO build
or Apple GPU.

The transient full-screen hot-pink startup frame persists for roughly one
second and then disappears before normal UI/gameplay. `SkipPregameVideos=1`
removes the logged video/logo states but not this frame, so the artifact remains
a separate low-impact early-presentation defect.

## The startup color is exact sRGB magenta in the ESO content surface

A user-captured Display P3 PNG measures the ESO frame rather than relying on a
subjective hot/neon-pink name. Its dominant stored P3 value `(234,51,247)`
converts exactly to sRGB `(255,0,255)` / `#FF00FF`. The 3420 x 2146 ESO content
is uniformly magenta over a large central region while macOS Game Mode and
screenshot overlays retain normal colors above it. The defect is therefore in
the ESO window/layer content, not global HDR, display color mapping, or overlay
composition.

The verified ESO executable contains 24 exact `{1,0,1,1}` float constants in
`__TEXT,__const`, while official MoltenVK 1.4.2 and the installed bridge contain
none. Seven decoded code references reach one pooled ESO copy. Several are in
a parameter initializer called from code paths that subsequently resolve the
literal techniques `technique_FXMaterial` and
`technique_FXMaterialTransparent`. This establishes that exact magenta is an
ESO FX-material default value, but no static call chain yet connects that
material to startup presentation. An FX-material error/default frame is thus a
locally supported hypothesis, not a confirmed writer.

Follow-up exact-build disassembly narrows that candidate. The initializer at
image offset `0x35fcd42` writes `(1,0,1,0)` at offsets `0x10` and `0x20` and
`(1,0,1,1)` at `0x30` in the same parameter block. Its only two direct calls
are inside builders that immediately select `technique_FXMaterial` and
`technique_FXMaterialTransparent`; embedded shader metadata also names
`cbFXMaterial`, the corresponding vertex/pixel shaders, and
`ZoFXMaterial.fx`. This confirms the constant's FX-material role, but a causal
startup association still requires the bounded intervention prepared in
Experiment 0025.

Experiment 0025 installed an exact entry hook for that initializer and retained
it through generation-2 present ordinal 180. The user observed unchanged pink,
but the hook recorded zero initializer calls before the bounded finish. No
sentinel value was therefore changed, and the visual result is inconclusive by
the predeclared decision table. The run does establish that this initializer is
not called between bridge hook installation and the end of early generation 2.
Any remaining version of the FX-material hypothesis must involve state created
before the hook or a different later value-copy/use path; repeating the same
initializer intervention cannot add evidence.

Exact-run correlation still establishes an early surface transition. In two
independent lifecycle traces, generation 1 is 3420 x 2148, has exactly one
successful acquire and present, and is replaced by generation 2 at
3420 x 2146 after 852 and 796 ms; another run repeats the reset timing at
908 ms. The screenshot corrects the former interpretation of that boundary:
its capture time is approximately 2.645 seconds after the corrected surface
was ready and 1.503 seconds after `AccountLogin`. Exact magenta therefore
persists or is reproduced beyond generation-1 replacement. A taller drawable
could be clipped into the corrected content area, so screenshot dimensions do
not independently prove which swapchain image was visible.

Static analysis shows that ESO creates these color attachments with load/store
operations and uses a separate full-target `vkCmdClearAttachments` path with a
dynamic four-float color. The first-generation proc trace reaches render-pass
and clear entry points before its one present, while shader-module creation,
graphics-pipeline creation, pipeline binding, and indexed drawing appear only
after generation 2 has presented. A missing material/texture shader is
therefore not a credible mechanism for the generation-1 submission. Because
the screenshot amendment proves that magenta remains visible after generation
2 is ready, this ordering does not exclude the exact-magenta FX-material path
from the complete interval.

An official MoltenVK 1.4.2 AppKit probe using the exact aggressive profile
recreates the two-pixel surface replacement without ESO. Five fresh load-only
processes read transparent black, and explicit opaque-black clears remain
black. The same controls pass at ESO's exact 3420 x 2148 to 3420 x 2146
extents; an explicit `{1,0,1,1}` control produces BGRA 255,0,255,255. MoltenVK
did not paint any tested first drawable pink in this configuration, and neither
the extent nor its two-pixel correction transforms a normal black clear. Fresh
load-only contents are not guaranteed by Vulkan; this observation constrains
the tested implementation rather than establishing a portable API default.

The leading remaining sources are an ESO-supplied dynamic clear, its exact
magenta FX-material default, and application window/layer background exposure.
Existing evidence does not record the submitted `VkClearAttachment.clearValue`
for both early generations or connect the material initializer to a presented
draw, so the precise pixel writer is not yet confirmed. A generation-1-only
diagnostic is insufficient.

Experiment 0024 closes the dynamic-clear branch with an exact live result. In
the run where the user again observed canonical magenta, the two-generation
audit linked one generation-1 and 180 generation-2 full-surface
`vkCmdClearAttachments` operations to successful submits and presents. Every
RGBA was opaque black `(0,0,0,1)`; no swapchain-linked render pass used
`LOAD_OP_CLEAR`. The audit finished exactly at generation-2 ordinal 180 without
reaching its detail cap, and the analyzer passed. A submitted Vulkan clear did
not supply the observed magenta pixels.

Experiment 0026 closes the post-swapchain branch with a direct final-image
read. In the exact run where the user observed pink, all twenty scheduled
samples completed before `vkQueuePresentKHR`. Generation 1 ordinal 1 and
generation 2 through ordinal 70 were opaque black at all five sampled points;
generation 2 ordinals 80 through 140 were exact RGBA `(255,0,255,255)` at all
five points; ordinals 150 through 180 contained ordinary scene colors. The
canonical magenta is therefore rendered into the final swapchain image. It is
not introduced by `CAMetalLayer`, CoreAnimation, overlay composition, HDR, or
display color mapping.

The same run narrows the application interval without yet naming one writer.
Immediately after the ordinal-70 black sample, ESO first obtains its descriptor
and graphics-pipeline functions, pipeline and vertex/index binding functions,
and `vkCmdDrawIndexed`; ordinal 80 is the first sampled magenta frame. Combined
with Experiment 0024's all-black submitted clears, a draw after the black clear
is the remaining writer class. The next evidence must identify the exact
swapchain-linked draw/pipeline signature; the prior FX-material initializer is
still only a candidate and is not promoted to a confirmed cause.

The remaining background alternative is also less consistent with the static
application code than previously known. ESO's `ZOMetalGameView` is opaque,
constructs a `CAMetalLayer`, and its `drawRect:` fills the view bounds with
`NSColor.blackColor` through `NSRectFill`. The executable contains no
`setBackgroundColor:` selector. Combined with the application-owned exact
magenta FX-material initializer and generation-2 pipeline/draw availability,
an ESO FX-material or related application draw is now the leading source. It
remains an inference rather than a confirmed writer until a presented draw or
pixel is directly associated with that path.

Experiment 0027 directly associates that interval with submitted draw
provenance. In the exact run where the user again observed pink, all nine
opaque-black samples through generation-2 ordinal 70 contained zero draws.
Every exact-magenta sample from ordinals 80 through 140 contained exactly one
indexed draw using pipeline signature `c43e4410d3b33fe7`, vertex shader hash
`c8307556011c995e`, and fragment shader hash `6907bd3576e3a930`. All twenty
pixel/draw pairs had complete submit-semaphore provenance and no capacity or
pipeline overflow. With the already verified black full-surface clears, this
single draw or an input it samples is the swapchain magenta writer.

The same draw signature, pipeline signature, and shader hashes remain present
at ordinals 150 through 180, when the sampled swapchain pixels are normal scene
colors. Pipeline identity therefore localizes the writer but does not prove
that the shader bytecode hard-codes magenta. Descriptor-bound texture/resource
content, uniform or push values, or another input can change without changing
the recorded pipeline. Descriptor/resource provenance for this exact identity
is the remaining causal boundary.

Experiment 0028 crosses that boundary at aggregate descriptor-update level.
In the exact run where the user again observed pink, all twenty scheduled
input samples completed with exact submit-semaphore provenance. The target
pipeline declares two descriptor sets containing two sampled-image descriptors
and six buffer descriptors in total, with no push-constant range. The pipeline
layout, required set count, bound descriptor-set handle signature, and zero
push state were identical across all seven exact-magenta and four later
normal-scene samples. Only the latest descriptor-update signature changed,
from `01922f8394b93e32` to `a7d448d22e640458`.

This establishes a descriptor-state transition, not merely a changing
pipeline or push value, at the magenta-to-scene boundary. The fingerprints
cover image view/sampler/layout and buffer handle/offset/range identities but
not resource memory contents. The result therefore weakens a pure in-place
content-fill explanation without proving which descriptor changed. The exact
remaining boundary is between the buffer-only three-descriptor set and the
mixed set containing two images and three buffers.

The target vertex hash is not material-specific: the same 17,392-byte module
was created 69 times in the bounded run. The target 18,280-byte fragment module
was created once immediately before the two target pipeline handles. Neither
exact module fingerprint appears as an uncompressed SPIR-V blob at any SPIR-V
magic candidate in the exact ESO executable or bundled `game0000.dat`. This
weakens a direct missing/corrupt standalone shader-file explanation, while not
excluding runtime translation or a compressed asset source.

A read-only inspection of the preserved 7,977,079-byte MoltenVK 1.4.2 pipeline
cache closes most of that semantic gap. The cache has exactly one shader entry
whose recorded module size is 18,280 bytes. Its retained MSL is a final
scene-and-GUI compositor: it samples `Sampler0` as scene color and `Sampler1`
as GUI color/alpha, converts the scene to sRGB, alpha-composites the GUI,
applies one scalar darkening factor, clips overscan to black, and writes the
result. It has exactly two texture inputs and three constant buffers. The
matching 17,392-byte vertex entry is a post-process fullscreen-quad transform
with three buffers and no texture input.

Those resource counts exactly map Experiment 0028's set 0 to the vertex
buffers and set 1 to the fragment compositor's two images and three buffers.
The fragment buffers cannot independently generate exact red and blue while
leaving green at zero; the canonical-magenta pixels must originate from the
scene image, GUI image, or their sampled combination. Uniform full-screen
output makes an ESO magenta placeholder/sentinel image the leading explanation,
not a broken shader. The aggregate descriptor transition still does not say
which image changes, and it does not exclude new contents written into a stable
image object. A bounded two-input image audit remains necessary before choosing
the neutralization point.

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

Experiment 0019 removed that defect from the execution path. Its exact user run
returned MoltenVK's original function pointers for all twelve lifecycle
functions and emitted no lifecycle event; only 114 startup and proc-routing
records were written for the complete process. The exact configuration also
used asynchronous queue submission and maximum concurrent compilation.

One 2048 x 1280 to 1920 x 1200 reset still produced solid-color output, with no
Vulkan error or crash. Those performance-path changes are therefore excluded
as a repair for the reset corruption. The user observed no obvious other issue,
but the run did not collect controlled timing data and cannot quantify a
performance gain.

## Live-resource checking has a measured descriptor-encode cost

MoltenVK 1.4.1 source shows that, with argument buffers disabled,
`LIVE_CHECK_ALL_RESOURCES=1` selects live-checking bind operations for every
descriptor binding. A changed texture, buffer, or sampler performs a hashed
`MVKLiveList::isLive()` lookup under an `os_unfair_lock`.

An M4 Rosetta probe timed the actual Vulkan-to-Metal submit encoding path with
20,000 draws alternating two valid texture descriptors. Across three balanced
processes and 21 samples per setting, the aggregate median fell from 195.833 to
176.090 ns per draw when the check was disabled, a 10.082% reduction in that
descriptor-heavy CPU interval. All final Metal pixels were correct.

The result does not predict an equal ESO FPS increase. It also does not remove
the correctness risk: the check intentionally avoids binding destroyed Metal
resources, and descriptors established before the prior audit's first
swapchain still lack complete content coverage.

## Shader compression targets retained MSL source, not shader execution

MoltenVK 1.4.1's `MVK_CONFIG_SHADER_COMPRESSION_ALGORITHM` compresses the MSL
source retained in memory for later pipeline-cache export. The already compiled
`MTLLibrary`, GPU resources, and shader execution are not compressed.

`MVKShaderLibrary` compresses converted MSL before retaining it and separately
compiles the uncompressed source. Reconstructing a shader library from cached
data decompresses the MSL before compilation. MoltenVK records both operations
as `mslCompress` and `mslDecompress` performance intervals. Compression can
therefore reduce retained source memory while adding CPU latency to shader
creation or cache reconstruction; it is not intrinsically an FPS optimization.
The default is no compression, and the documented fastest candidate is LZ4,
with the largest compressed representation among the provided algorithms.
