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

## Known architectural risk

Vulkan handles are runtime-owned opaque objects. If ESO creates a handle through
new MoltenVK and later passes it into an unredirected old MoltenVK wrapper, a
crash is expected. Direct-call scanning found 39 external calls to 16 Vulkan
entry points, but it did not prove the absence of address-taken or table-based
references. Exhaustive cross-reference coverage is required.

