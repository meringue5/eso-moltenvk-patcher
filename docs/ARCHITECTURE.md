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

## Remaining architectural risk

Vulkan handles remain runtime-owned opaque objects. The analysis substantially
reduces the obvious public-wrapper mixing risk for this exact executable, but
does not prove callback, private ABI, extension-negotiation, object-lifetime,
or surface/swapchain compatibility. Any target update invalidates the active
coverage. The fast update audit can re-establish the static portion only when
the complete profiled layout is unchanged; non-game probes and the bounded
runtime gate are still required.
