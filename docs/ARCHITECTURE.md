# Bridge architecture

## Why a proxy is needed

ESO loads `@executable_path/libBink2Macx64.dylib` dynamically, while MoltenVK is
statically linked into the main executable. The prototype replaces the Bink
library with a small x86_64 proxy that:

1. Re-exports every symbol from a renamed pristine Bink library.
2. Runs a constructor before ESO's normal startup.
3. Validates the main executable UUID and patch-site bytes.
4. Loads official `libMoltenVK.dylib` with `dlopen`.
5. Redirects selected old Vulkan wrapper entries with 12-byte absolute jumps.
6. Routes GIPA requests for device-extension enumeration, surface-format
   enumeration, and device creation through a narrow compatibility layer.

Mach-O text pages do not accept added write permission through ordinary
`mprotect`. The prototype uses `mach_vm_protect` with `VM_PROT_COPY`, writes to a
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

## Remaining architectural risk

Vulkan handles remain runtime-owned opaque objects. The analysis substantially
reduces the obvious public-wrapper mixing risk for this exact executable, but
does not prove callback, private ABI, extension-negotiation, object-lifetime,
or surface/swapchain compatibility. Any target update invalidates the active
coverage. The fast update audit can re-establish the static portion only when
the complete profiled layout is unchanged; non-game probes and the bounded
runtime gate are still required.
