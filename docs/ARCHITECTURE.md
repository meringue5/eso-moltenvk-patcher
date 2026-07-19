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

## Verified coverage for the current target

The current analyzer enumerates old Vulkan text symbols and checks direct calls
and jumps, RIP-relative address-taking, absolute immediates, and dyld rebase
pointers. For the fingerprinted ESO build it found 40 external references to 17
entry points: 39 calls/jumps to 16 wrappers plus one address-taken
`vkGetInstanceProcAddr`. This exactly matches the redirect manifest, and all 17
are exported by MoltenVK 1.4.1.

ESO directly queries 17 unique names through GIPA and 65 through GDPA. A
100-name old/new non-game probe found no case where 1.0.18 returned non-null and
1.4.1 returned null on the route ESO actually uses. The bridge now traces both
proc-address functions so a controlled run can verify the dynamic sequence.

## Remaining architectural risk

Vulkan handles remain runtime-owned opaque objects. The analysis substantially
reduces the obvious public-wrapper mixing risk for this exact executable, but
does not prove callback, private ABI, object-lifetime, or surface/swapchain
compatibility. Any target update invalidates the coverage until fingerprints,
offsets, cross-references, and proc behavior are re-established.
