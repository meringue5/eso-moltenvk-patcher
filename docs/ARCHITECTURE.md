# Bridge architecture

## Why a proxy is needed

ESO loads `@executable_path/libBink2Macx64.dylib` dynamically, while MoltenVK is
statically linked into the main executable. The production bridge replaces the Bink
library with a small x86_64 proxy that:

1. Re-exports every symbol from a renamed pristine Bink library.
2. Runs a constructor before ESO's normal startup.
3. Validates the main executable UUID and patch-site bytes.
4. Loads official `libMoltenVK.dylib` with `dlopen`.
5. Redirects selected old Vulkan wrapper entries with 12-byte absolute jumps.
6. Routes GIPA requests for device-extension enumeration, surface-format
   enumeration, and device creation through a narrow compatibility layer.

Mach-O text pages do not accept added write permission through ordinary
`mprotect`. The bridge uses `mach_vm_protect` with `VM_PROT_COPY`, writes to a
private page copy, clears the instruction cache, and restores RX permission.
This mechanism was independently verified under Rosetta.

## Safety boundaries

- The bridge is tied to one exact ESO SHA-256 and Mach-O UUID.
- Every target's original bytes are verified before any patch is written.
- All target symbols in the new runtime are resolved before pages are changed.
- Installation preserves the pristine Bink library and old pipeline cache.
- Unknown game builds fail closed.

## Launcher-update gate

`config/current-target.txt` selects the default manifest used by status,
build, install, restore, and update tooling. This removes dated manifest names
from shell scripts and makes a target promotion one explicit pointer change.
`scripts/check-update.sh` compares SHA-256, Mach-O UUID, client version, and
databuild and returns a nonzero result for unknown, historical, content-updated,
or internally inconsistent identities.

`scripts/quick-update-check.sh` combines that identity with a fresh, completed
launcher `Live_Prod` repository comparison and the installed bridge
target/marker. Routine `READY` checks end there; `STOP` selects the slower
component diagnosis. Evidence preparation and collection save the exact
component results, repository IDs, settings hashes, and controlled setting
values automatically.

The schema-v2 current manifest also owns a compact static-layout profile. It
records both embedded archive-member hashes, the linked MoltenVK object and
symbol count, exact patch bytes, reference shapes, proc-query routes, and the
pinned replacement-runtime identity. `scripts/rebase-update.sh` recomputes
that profile on the updated bundle and emits a new manifest only for an exact
match. Files are rehashed before promotion to close the launcher-update race.

This mechanism automates the unchanged-layout case; it does not infer new
offsets or relax a mismatch. It writes repository configuration only. Loader
restore, build, installation, and runtime validation remain separate gates.

## Verified coverage for the current target

The current analyzer enumerates old Vulkan text symbols and checks direct calls
and jumps, RIP-relative address-taking, absolute immediates, and dyld rebase
pointers. For the fingerprinted ESO build it found 40 external references to 17
entry points: 39 calls/jumps to 16 wrappers plus one address-taken
`vkGetInstanceProcAddr`. This exactly matches the redirect manifest, and all 17
are exported by MoltenVK 1.4.1.

ESO directly queries 17 unique names through GIPA and 65 through GDPA. A
100-name old/new non-game probe initially found no old-nonnull/new-null case,
but Experiment 0002 exposed a conditional route: MoltenVK 1.4.1 advertises
`VK_EXT_hdr_metadata`, and its `vkSetHdrMetadataEXT` GDPA result is non-null only
when that extension was enabled on the device. The bridge trace recorded a NULL
result for this proc immediately before the repeated startup crash sequence.

Future compatibility probes must model extension enumeration and device
creation as one state transition. Comparing names against an arbitrary device
is insufficient when the replacement runtime advertises capabilities absent
from the embedded runtime.

## HDR compatibility layer

Embedded MoltenVK 1.0.18 does not advertise `VK_EXT_hdr_metadata`, while
MoltenVK 1.4.1 does. The compatibility layer removes only that exact name from
device-extension enumeration. It preserves every other property and its order,
implements count/data and `VK_INCOMPLETE` behavior, and leaves layer-specific
enumeration unchanged.

The `vkCreateDevice` wrapper logs the exact enabled-extension list and forwards
the original `VkDeviceCreateInfo` unchanged. An explicit HDR request is still
forwarded; Experiment 0004 instead expects ESO not to request the extension and
not to query `vkSetHdrMetadataEXT` through GDPA.

Experiment 0004 proved that filtering the device-extension advertisement does
not control ESO's HDR branch. ESO independently inspects surface formats. For
the fingerprinted executable, the exact pair
`VK_FORMAT_A2B10G10R10_UNORM_PACK32` plus
`VK_COLOR_SPACE_HDR10_ST2084_EXT` sets the object flag that guards an unchecked
`vkSetHdrMetadataEXT` call. Embedded MoltenVK 1.0.18 does not expose this pair
on the test M4; MoltenVK 1.4.1 does.

The Experiment 0005 source candidate also wraps
`vkGetPhysicalDeviceSurfaceFormatsKHR` and removes only that exact pair. It
preserves every other entry and order and implements count/data and
`VK_INCOMPLETE` behavior. It deliberately does not provide a fake setter or
remove every wide-color/HDR format.

The raw enumeration, surface-format, device-creation, GIPA, and GDPA
destinations are all resolved before any code page becomes writable. Proc
lookup only selects a wrapper after that initialization; it does not mutate
those destinations.

Each bridge log line carries a UTC nanosecond run timestamp and PID. The startup
checker groups records by that identity, applies the experiment preparation
time as a lower bound, and fails closed on missing filter evidence, HDR enable,
an HDR device-proc query, or bridge errors.

Evidence collection keeps that time-gated verdict authoritative. If and only if
no run matches the preparation boundary, it also emits a clearly separate
retrospective verdict for post-mortem attribution; a failed eligible run is
never replaced by an older pass. It also preserves the full before/after
settings files in the ignored evidence directory and extracts
`DeviceWaitIdle -> fpCreateSwapchainKHR -> OnDeviceReset` summaries. The
settings comparison tool requires exact line structure and key identity before
reporting value changes.

The real MoltenVK 1.4.1 non-game probes report raw HDR device advertisement,
filtered visible absence, HDR disabled at device creation, non-null GIPA, NULL
GDPA, and exact surface-format removal from 60 raw to 59 visible entries. That
proves the local compatibility transitions and successful device creation; it
does not prove ESO startup behavior.

## Descriptor compatibility mode

Experiment 0006 adds a fail-closed `descriptor-compat` marker mode. Before
loading MoltenVK it retains the live-resource check and disables only Metal
argument buffers. After `dlopen`, the bridge queries
`vkGetMoltenVKConfigurationMVK` and refuses to patch unless the effective
runtime reports live-resource checking enabled, argument buffers disabled, and
the controlled MTLHeap, queue-submit, command-pooling, and prefill values still
at the 1.4.1 defaults used by Experiment 0005.

This check distinguishes the effective runtime state from merely recording
successful `setenv` calls. A separate non-game probe verifies both the clean
1.4.1 default and the descriptor-compatible state in independent processes.
Neither check establishes rendering correctness; only the staged ESO run can
answer that question.

## Legacy core-feature profile

Experiment 0018 adds a `legacy-feature-profile` mode above descriptor
compatibility. GIPA returns a wrapper for `vkGetPhysicalDeviceFeatures`; the
wrapper calls MoltenVK 1.4.1 and clears only the 18 core features that the
embedded 1.0.18 runtime reports as false on the target Apple M4. It leaves all
other feature bits and every physical-device property unchanged.

ESO passes the returned structure directly to `vkCreateDevice`. The existing
device wrapper therefore performs a second fail-closed check in this mode: if
any of the 18 prohibited fields is enabled, it returns
`VK_ERROR_FEATURE_NOT_PRESENT` without calling MoltenVK. Startup validation
requires the exact 36-to-18 query summary, the feature GIPA route, and an
18-feature create request with zero prohibited fields.

This wrapper is invoked only during device discovery and creation. It adds no
descriptor, draw, submit, or presentation hot-path work.

## Performance-safe execution path

Experiment 0019 adds a `performance-safe` mode that retains descriptor
compatibility and the HDR filters but removes diagnostic routing from the
steady-state execution path. The lifecycle interceptor is disabled before
proc lookup; all twelve lifecycle-observed functions therefore return the
exact MoltenVK pointer instead of a bridge wrapper. Startup validation requires
those GDPA records to report `shim=none` and rejects any lifecycle event.

Before MoltenVK is loaded, the mode sets asynchronous queue submission and
maximum concurrent compilation. The post-load private configuration query
requires those values together with live-resource checking, disabled argument
buffers, MTLHeap where safe, command pooling, and no command prefill. Any
mismatch stops before ESO's patch sites are modified.

This mode deliberately does not use Experiment 0018's feature mask and does
not disable live-resource checking. It combines performance-path changes to
minimize scarce user executions; it is not a single-variable attribution
experiment.

Experiment 0020 adds `performance-aggressive` as an exact derivative of this
mode. It changes only `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES` from `1` to `0`.
The same direct lifecycle routing and asynchronous/concurrent settings remain.
The post-load configuration query requires live-resource checking to be
disabled; a safe-mode value or any other mismatch stops before patching ESO.

This removes MoltenVK's per-changed-resource `MVKLiveList::isLive()` locked
lookup from the argument-buffer-disabled descriptor encode path. It also
removes the protection that skips a destroyed Metal resource still referenced
by a descriptor, so the mode remains distinct and explicitly aggressive.

## Legacy allocation mode

Experiment 0011 adds a distinct `legacy-allocation` marker mode. It retains the
descriptor-compatible state and sets only `MVK_CONFIG_USE_MTLHEAP=0`. The
post-load configuration query must report live-resource checking `1`, argument
buffers `0`, MTLHeap `0`, synchronous submission `1`, command pooling `1`, and
prefill `0`; otherwise the bridge refuses to patch ESO.

This mode does not replace MoltenVK 1.4.1 or alter Vulkan calls. It isolates
MoltenVK's Metal-heap resource placement/reuse path while retaining the
observation-only swapchain lifecycle wrappers.

## No-command-pooling mode

Experiment 0013 adds a fail-closed `no-command-pooling` marker mode. It retains
the complete Experiment 0012 compatibility and bounded reset-resource trace,
keeps MTLHeap at `where safe`, and sets only
`MVK_CONFIG_USE_COMMAND_POOLING=0`. The post-load query must report command
pooling `0` with live-resource checking `1`, argument buffers `0`, MTLHeap `1`,
synchronous submission `1`, and prefill `0`; otherwise the bridge refuses to
patch ESO.

This is a correctness counterfactual for MoltenVK's reuse of internal command
objects. It does not change Vulkan calls, ESO's command-pool API objects, or
the replacement runtime version. Disabling pooling can add CPU allocation
overhead and is not treated as a performance configuration unless rendering
first passes.

## Reset lifecycle trace

Experiment 0010 adds observation-only wrappers to selected device functions
returned through GDPA. The static 17-entry redirect set, Vulkan arguments,
results, HDR filters, and effective MoltenVK configuration remain unchanged.
The wrappers assign a generation to every successful swapchain creation and
connect enumerated swapchain images to tracked image views, render passes, and
framebuffers.

The trace records `vkDeviceWaitIdle`, replacement extent and old-swapchain
identity, dependent resource creation/destruction, and the first eight image
acquires and presentations for each generation. Non-success acquire/present
results are always recorded. Destroying a generation while one of its tracked
image views or framebuffers remains live is emitted as an analyzer anomaly.

Tracking has fixed capacities and fails visibly with `LIFECYCLE_ERROR` rather
than silently claiming complete coverage. The wrappers use a mutex around
tracking state and do not retain pointers to caller-owned create structures.
The coded post-run analyzer selects only a run after the evidence-preparation
boundary.

## Bounded reset-resource trace

Experiment 0012 composes a second observation layer outside the lifecycle
wrappers. It also intercepts resource-related entries in the 17 direct patch
targets, because relying on GDPA alone would miss ESO's static memory, buffer,
image, command-buffer-end, and render-pass-end calls.

The trace remains inactive during ordinary startup and gameplay. After two
swapchains exist, the next successful device wait arms one bounded window. The
next created swapchain becomes the reset target, and its eighth presentation
atomically closes the window and emits aggregate counters. Only the first 48
high-value details are logged. All wrappers call the previous layer and return
its result unchanged.

This architecture establishes whether reset-time resources and command work
are created, bound, recorded, and submitted. It cannot by itself inspect Metal
resource contents or prove that a successful Vulkan operation produced correct
pixels.

## Bounded render-graph audit

Experiment 0014 composes a fixed-capacity state mirror with the existing
eight-presentation window. Low-frequency image, image-view, render-pass,
framebuffer, pipeline, and descriptor-set lifetimes are retained from startup.
Image descriptor contents begin mirroring at the first successful swapchain,
before the loaded-world reset under test.

Descriptor sets and slots use open-addressed handle hashes. Destroying an
image view marks every mirrored slot that references it; binding a set then
reports its stale slot count and last update sequence without rescanning the
whole slot table. Descriptor-pool reset/free invalidation is batched. The
mirror allocates no memory in Vulkan wrappers and exposes any fixed-table
overflow as an analyzer failure.

The bounded active window additionally records:

- live `VkImage` memory ranges and undeclared overlap;
- reuse of overlapping ranges after an image was destroyed, with the old
  destruction sequence retained;
- image barrier old/new layouts;
- framebuffer image views joined to render-pass initial/final layouts;
- reset-created pipeline cache, render pass, subpass, and later binds;
- image copy, blit, and resolve call counts.

For each of the first 128 bounded render passes, the audit emits the tail state:
the last graphics pipeline and last descriptor-set batch seen before
`vkCmdEndRenderPass`, together with each mirrored set's last update sequence
and stale count. This samples the composite-facing end of a pass rather than
letting thousands of per-draw binds consume the log budget.

All wrappers forward the original call first where its arguments must remain
valid, preserve the original result, and make no Vulkan state correction.
Layout state is observed in API command-recording order, which can differ from
queue execution order across command buffers. A mismatch therefore identifies
a chain for focused inspection; it is not automatically classified as a
Vulkan violation.

## Reset-only pipeline-cache bypass

Experiment 0015 adds an explicitly selected mode above the same bounded audit.
Before the trace begins, pipeline creation is forwarded unchanged. While the
loaded-world reset window is active, a non-null optional cache passed to
`vkCreateGraphicsPipelines` is replaced with `VK_NULL_HANDLE`; all pipeline
create infos, allocation callbacks, output handles, and results remain
unchanged.

Each changed call emits the original cache handle, forwarded null handle,
pipeline count, and result. The audit receives the forwarded cache value, so
its aggregate must report zero reset pipelines created with a cache. The coded
analyzer also requires the sum of bypass records to equal the reset graphics
pipeline count. The mode does not modify either on-disk cache file and does not
disable cache use during startup or steady-world rendering.

## Full-lifetime reset-state audit

Experiment 0016 selects `full-lifetime-audit` and removes the pipeline-cache
counterfactual. The descriptor mirror is enabled at process sequence 1 rather
than at the first swapchain. Descriptor-set layout records retain binding
types, array sizes, and immutable samplers; set slots retain image view,
sampler, buffer, buffer view, range, and image-layout contents. Bound sets
report known and unknown slot totals. A zero update sequence is explicitly
`content=unknown`, never a clean descriptor result.

The same fixed-capacity mirror records command-buffer ownership and generation
across allocation, free, explicit reset, pool reset, begin, end, record, and
submit. Each bounded submit reports its queue ordinal and the command
generation's reset/begin/end/bind/update sequences. Descriptor contents changed
after recording are reported separately rather than silently folded into the
record-time state.

Render-pass records retain color, resolve, input, depth/stencil, and preserve
roles; first and last subpass use; load/store and stencil operations; and
whether a required clear value was supplied. Image barriers expand into
aspect/mip/layer records with their stage and access masks. Transitions are
retained on the recording command generation and applied to the subresource
mirror only when that generation is submitted, so layout comparisons follow
queue execution order instead of global API recording order.

Semaphore records join swapchain acquisition, queue waits/signals, and present
waits. Fence records join creation flags, submit, reset, and wait results.
These tables, descriptor-slot samples, barrier samples, command-submit samples,
and render-pass tails are all bounded. Wrappers allocate no dynamic memory and
ordinary hot paths do not write one log record per call. Any overflow, missing
layout, unknown bound slot/resource, invalid command generation, or unknown
synchronization edge makes the coded result inconclusive.

## Swapchain Metal texture-cache backport

Experiment 0017 keeps the MoltenVK 1.4.1 API and configuration baseline but
builds that exact source tag with one later upstream correction. A presentable
swapchain `VkImage` does not own one permanent Metal texture:
`MVKPresentableSwapchainImage::getMTLTexture()` obtains the texture from the
current `CAMetalDrawable`. In unpatched 1.4.1,
`MVKImageViewPlane::getMTLTexture()` can nevertheless retain a derived Metal
texture view created from an earlier drawable.

The backport checks the current base, parent/root texture, and IOSurface
identity before reusing the derived view. If the backing drawable changed, it
removes the old view from live-resource tracking, releases it, and creates a
new view from the current base. No Vulkan call, swapchain policy, descriptor
configuration, or pipeline cache is changed.

The build script verifies the exact v1.4.1 source commit, exact upstream commit,
and exact two-file patch hash before compiling. The differential probe renders
through a view, reads the current drawable base texture, and requires official
1.4.1 to expose the stale write while the backport corrects it.

## Early startup compositor path

Experiments 0022 through 0028 establish the startup path from the first macOS
surface to final pixels. ESO first creates a 3420 x 2148 swapchain, presents it
once, and replaces it with a 3420 x 2146 swapchain after roughly 0.8--0.9
seconds. The visible magenta continues on the corrected swapchain. Submitted
full-surface clears remain opaque black, and the final swapchain is likewise
black through generation-2 present ordinal 70.

After ESO obtains its descriptor, pipeline, and draw entry points, one indexed
fullscreen draw begins at ordinal 80. That draw uses stable pipeline signature
`c43e4410d3b33fe7`, vertex module hash `c8307556011c995e`, and fragment module
hash `6907bd3576e3a930`. It writes canonical magenta through ordinal 140 and
ordinary scene pixels from ordinal 150 onward without changing the pipeline,
draw signature, descriptor-set objects, layout, or push state. Only the latest
descriptor-update aggregate changes.

The preserved MoltenVK 1.4.2 pipeline cache supplies the missing semantic
layer. Its only 18,280-byte shader-module entry corresponds to the target
fragment module created once in the exact run. The retained MSL is ESO's final
scene-and-GUI compositor: it samples a scene texture and a GUI texture, applies
scene gamma conversion, composites with GUI alpha, applies one scalar scene
darkening value, clips overscan to black, and writes the final color. Its three
buffers cannot independently create red-and-blue with zero green. The matching
17,392-byte vertex module is a post-process fullscreen-quad transform using
three buffers and no image.

This maps Experiment 0028's buffer-only set 0 to vertex inputs and its mixed
set 1 to the fragment compositor's two images plus three buffers. The remaining
pixel-source boundary is therefore the compositor's scene image versus GUI
image, including the possibility that a stable image object receives new
contents. A shader compilation failure, a MoltenVK default color, a Vulkan
clear, and the post-swapchain compositor/display path no longer fit the complete
evidence chain.

The production repair should remain startup-bounded. First identify which of
the two compositor images supplies magenta and the exact resource transition
that makes it valid. Prefer neutralizing that application placeholder or
withholding only its presentation; permanently latch back to direct MoltenVK
forwarding at the first valid compositor input. Do not add steady-state pixel
readback or a general shader/material replacement.

The complete observed presentation stack is now:

1. ESO's `ZOMetalGameView` owns an opaque `CAMetalLayer`; its AppKit fallback
   paint is black.
2. MoltenVK exposes that layer as a Vulkan surface and two successive ESO
   swapchains. MoltenVK load-only content is transparent black, not magenta.
3. ESO's submitted startup clears write opaque black to the swapchain.
4. ESO renders offscreen scene and GUI images, then the identified fullscreen
   compositor samples them as MSL `Sampler0` and `Sampler1` and writes the
   opaque final swapchain color.
5. `vkQueuePresentKHR` hands that already-magenta image to the Metal layer.
   Normally colored Game Mode and window overlays are composed afterward by
   macOS.

Experiment 0029's isolated diagnostic crosses the remaining offscreen
boundary. It preserves descriptor image-view identity and the view's base
mip/layer, then uses MoltenVK's Metal texture accessor only at the twenty
proven pre-present samples. It supports BGRA8, RGBA8, and RGBA16F, sorts the
two image bindings into MSL sampler order, and compares direct input pixels
with descriptor signatures. This distinguishes scene versus GUI and descriptor
replacement versus in-place content. Any image-view gap, descriptor copy,
unsupported subresource/format, or synchronization loss makes the run
inconclusive. The mechanism is absent from normal production modes.

## Remaining architectural risk

Vulkan handles remain runtime-owned opaque objects. The analysis substantially
reduces the obvious public-wrapper mixing risk for this exact executable, but
does not prove callback, private ABI, extension-negotiation, object-lifetime,
or surface/swapchain compatibility. Any target update invalidates the active
coverage. The fast update audit can re-establish the static portion only when
the complete profiled layout is unchanged; non-game probes and the bounded
runtime gate are still required.
