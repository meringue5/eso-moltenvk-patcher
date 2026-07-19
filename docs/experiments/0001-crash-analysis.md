# Experiment 0001 crash analysis

- Date analyzed: 2026-07-19
- Related run: [MoltenVK 1.4.1 full redirect](0001-moltenvk-1.4.1-full-redirect.md)

## Confirmed crash facts

- Launch: 2026-07-19 13:04:43 KST
- Crash: 2026-07-19 13:04:46.219 KST
- Platform: macOS 26.5.2, Apple M4, x86_64 translated by Rosetta
- Exception: `EXC_BAD_ACCESS / SIGSEGV`
- Fault: instruction pointer `0x0`, `KERN_INVALID_ADDRESS`
- Faulting queue: `com.apple.main-thread`
- New MoltenVK 1.4.1 dylib was present in the loaded-image list.

The last ESO unified-log event was a Metal compiler warning at the same
millisecond as the fault. Process teardown began about 3 ms later. This does not
prove that the Metal compiler caused the crash; it establishes that the failure
occurred during very early graphics/shader initialization.

## Relevant call chain

The first recoverable ESO return address was executable offset `0x364a7a5`
(unslid `0x10364a7a5`), immediately after an indirect call in the Vulkan instance
initialization path:

```asm
0x10364a79c  movq  (%r14), %rax
0x10364a79f  movq  %r14, %rdi
0x10364a7a2  callq *0x28(%rax)
0x10364a7a5  movb  $0x1, %bl
```

Nearby error handling contains the string:

```text
Vulkan: Unspecified error %d when trying to initialize the instance.
```

The crash state has `RIP=0`, which is consistent with an indirect call or tail
call eventually reaching a null function pointer. The report is not symbolized
well enough to claim exactly which Vulkan function was null.

## Updated diagnosis

This makes a missing or incompletely populated function table more likely than
a generic GPU hang. The next bridge build wraps `vkGetInstanceProcAddr` and logs
every requested name and returned address, marking null results. A new launch
must not be attempted until this tracing build and live-resource compatibility
mode are reviewed together.

The full `.ips` is intentionally not committed because it contains local paths
and persistent device/report identifiers.

## 2026-07-19 amendment after Experiment 0002

Experiment 0002 reproduced `RIP=0` with the same first recoverable ESO return
offset, `0x364a7a5`. Its complete proc trace ended with a NULL GDPA result for
`vkSetHdrMetadataEXT`. A non-game probe then showed that MoltenVK 1.4.1
advertises `VK_EXT_hdr_metadata` but returns that device proc only when the
extension is enabled on the device. This supersedes the earlier broad
missing-function-table diagnosis with a narrower extension-negotiation lead;
it still does not prove which null pointer the indirect call reached.
