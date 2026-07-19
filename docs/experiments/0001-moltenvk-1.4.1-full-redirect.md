# Experiment 0001: MoltenVK 1.4.1 full redirect

Date: 2026-07-19  
Result: **Failed after bridge activation; restored successfully**

## Goal

Replace ESO's effective MoltenVK 1.0.18 runtime without bypassing Steam launch or
authentication.

## Preflight results

- Official MoltenVK 1.4.1 universal dylib loaded as x86_64 under Rosetta.
- `vkCreateInstance` succeeded with Vulkan 1.0, `VK_KHR_surface`, and
  `VK_MVK_macos_surface`, without a portability enumeration flag.
- One Apple M4 physical device was enumerated.
- `VK_KHR_swapchain`, `VK_KHR_maintenance1`,
  `VK_AMD_negative_viewport_height`, and `VK_EXT_debug_marker` were present.
- `vkCreateDevice` succeeded with the ESO-era extension set.
- Bink proxy `dlopen` and `BinkOpen` re-export lookup succeeded.
- A separate Rosetta self-patch probe changed a function result from 1 to 2.

## Redirect set

The bridge redirected `vkGetInstanceProcAddr` plus 16 entry points reached by 39
direct ESO calls: memory allocation/mapping, buffer/image creation and binding,
memory-requirement queries, physical-device property queries,
`vkEndCommandBuffer`, and `vkCmdEndRenderPass`.

## Runtime result

The bridge log recorded:

```text
ACTIVE: redirected 17 Vulkan entry points to MoltenVK 1.4.1
```

ESO then crashed about 3.1 seconds after process launch. The crash report was
written later to the system DiagnosticReports directory rather than the user's
directory. It records `EXC_BAD_ACCESS / SIGSEGV`, `RIP=0`, on the main thread
during early Vulkan/Metal initialization. See [Crash analysis](../CRASH_ANALYSIS.md).
The proxy, new runtime, and new pipeline cache were removed from the active
path, and the pristine Bink loader plus old pipeline cache were restored.

## Leading hypotheses

1. A requested Vulkan function or initialization callback was null or left
   unpopulated. The next build traces every `vkGetInstanceProcAddr` result.
2. An unobserved pointer/table reference called an old MoltenVK wrapper with a
   new MoltenVK handle.
3. MoltenVK 1.4.1's stricter descriptor lifetime implementation rejected an ESO
   behavior tolerated by 1.0.18.
4. A surface, swapchain, or configuration path differs despite basic instance
   and device compatibility.
5. A removed private MoltenVK ABI is reached indirectly after startup.

## Next experiment gate

Do not repeat a full gameplay launch unchanged. Add crash capture, enable the
live-resource compatibility option for one startup test, and make wrapper
cross-reference analysis exhaustive first.
