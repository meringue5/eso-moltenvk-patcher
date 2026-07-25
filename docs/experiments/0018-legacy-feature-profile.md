# Experiment 0018: embedded-runtime core feature profile

- Date: 2026-07-26
- Outcome: **failed; exact embedded feature profile still produced solid output**
- Rollback: **checked pristine loader available; both pipeline caches preserved**

## Question

Does ESO enable MoltenVK 1.4.1 core device features that its embedded MoltenVK
1.0.18 reported as unsupported, thereby selecting a render or synchronization
path that fails after a loaded-world graphics reset?

## Hypothesis

ESO obtains `VkPhysicalDeviceFeatures` twice. It first tests ten required
features to select a physical device. It later passes the complete returned
feature structure unchanged as `VkDeviceCreateInfo.pEnabledFeatures`.

On the Apple M4, MoltenVK 1.4.1 reports 18 features as true that embedded
1.0.18 reported as false. All ten selection requirements are true in both
runtimes, so hiding only the 18 additions preserves device eligibility while
making the created device's core feature set identical to the embedded
runtime. If one of the added capabilities selects the faulty reset path, one
loaded-world resolution change should remain rendering-correct with this mask.

## Static target evidence

The selected ESO 12.0.7 executable has one device-initialization routine
covering the relevant calls:

- `0x10364aa89` resolves `vkGetPhysicalDeviceFeatures` through GIPA and the
  returned function fills a 55-field structure;
- `0x10364aaa6` through `0x10364ab16` tests ten fields:
  `independentBlend`, `dualSrcBlend`, `depthClamp`, `depthBiasClamp`,
  `fillModeNonSolid`, `samplerAnisotropy`, `textureCompressionBC`,
  `fragmentStoresAndAtomics`, `shaderImageGatherExtended`, and
  `shaderClipDistance`;
- `0x10364af24` resolves and calls `vkGetPhysicalDeviceFeatures` again for the
  selected device;
- `0x10364b4a7` stores that complete structure at the
  `VkDeviceCreateInfo.pEnabledFeatures` offset;
- `0x10364b4f3` invokes `vkCreateDevice`.

All ten selection fields are true under both embedded 1.0.18 and official
1.4.1. The static control flow therefore shows that the new fields affect
device enablement, not which single Apple M4 physical device is selected.

## Target and change set

- Keep official MoltenVK 1.4.1 and its existing pipeline-cache identity.
- Keep live-resource checks enabled, Metal argument buffers disabled, MTLHeap
  `where safe`, synchronous submission enabled, command pooling enabled, and
  command prefill disabled.
- Add the distinct marker mode `legacy-feature-profile`.
- Route only GIPA's `vkGetPhysicalDeviceFeatures` result through a wrapper.
- Call the real 1.4.1 function first, then clear exactly these 18 fields that
  are false in the embedded M4 profile:
  `robustBufferAccess`, `fullDrawIndexUint32`, `tessellationShader`,
  `sampleRateShading`, `drawIndirectFirstInstance`, `multiViewport`,
  `textureCompressionETC2`, `textureCompressionASTC_LDR`,
  `shaderTessellationAndGeometryPointSize`,
  `shaderStorageImageReadWithoutFormat`,
  `shaderStorageImageWriteWithoutFormat`,
  `shaderUniformBufferArrayDynamicIndexing`,
  `shaderSampledImageArrayDynamicIndexing`,
  `shaderStorageBufferArrayDynamicIndexing`,
  `shaderStorageImageArrayDynamicIndexing`, `shaderInt64`,
  `shaderResourceMinLod`, and `inheritedQueries`.
- At `vkCreateDevice`, fail with `VK_ERROR_FEATURE_NOT_PRESENT` before
  forwarding if any of those 18 fields is nevertheless enabled.
- Do not spoof vendor/device identity, limits, device name, or pipeline-cache
  UUID. Do not enable the full-lifetime audit or add hot-path logging.

## MoltenVK 1.4.2 classification

An exact x86_64 source build of tag commit
`db66022459ffb663aa2b50f6b018bc2e124f5edf` reports MoltenVK 1.4.2 and has
normalized dylib SHA-256
`808a5d427a03c6b643fbb03c5a4d0fca3db959df90ebdaeb96123bbce23a8a69`.

With the bridge's real argument-buffer-disabled configuration, complete
`VkPhysicalDeviceFeatures`, `VkPhysicalDeviceLimits`, and sparse properties
are identical between 1.4.1 and 1.4.2. Only the advertised Vulkan API version,
driver version, and pipeline-cache UUID differ. The 1.4.2 UUID would cold-start
a separate cache; this experiment avoids that confound and preserves both
existing caches.

The remaining 1.4.2 changes were statically classified against the captured
ESO path. ESO enables no descriptor-indexing or external-memory extension,
uses no argument buffers, has antialiasing disabled, and its captured reset
images are single-sampled and non-transient. The memoryless attachment,
MSAA-resolve, descriptor-alignment/indexing, external-memory, primitive
restart, 1 x 1 swapchain, and heap-placement fixes therefore do not match the
failing reset. The subpass barrier additions are inactive without argument
buffers, and the observed proc profile does not use begin/end visibility
queries.

## Non-game reset composite

`tools/probe_reset_composite.mm` allocates one descriptor set before any render
target, then alternates exact 2048 x 1280 and 1920 x 1200 source images for 24
cycles. Every cycle recreates the image, view, render pass, framebuffer, and
graphics pipeline; updates the same descriptor set; alternates
`vkResetCommandBuffer` and `vkResetCommandPool`; renders to an offscreen image;
samples it into a fixed output; waits; and validates the Metal pixel.

Official 1.4.1 and the exact 1.4.2 build both passed all 24 cycles with the
expected pixel each time. This weakens a minimal long-lived descriptor,
image-view replacement, command-buffer reuse, or render-pass dependency
failure and provides no differential reason to install 1.4.2.

## Device-profile probes

`tools/probe_vulkan.c` now emits every core feature, every Vulkan 1.0 limit,
sparse properties, and device identity. The automated M4 gate
`scripts/probe-device-feature-profile.sh` compares embedded 1.0.18, official
1.4.1, the masked 1.4.1 path, and exact 1.4.2.

The gate proves:

- embedded 1.0.18 has 18 enabled core features;
- official 1.4.1 has 36 enabled core features;
- the exact 18-field mask reduces 1.4.1 to 18;
- all 55 masked 1.4.1 feature values are byte-for-byte text-equivalent to the
  embedded profile;
- ESO-era device creation succeeds with those 18 features;
- a prohibited feature in `pEnabledFeatures` fails closed before reaching the
  real device creator;
- 1.4.1 and 1.4.2 core features and limits are identical.

The shadow-bundle source build passed the Bink re-export lookup, Rosetta
self-patch probe, lifecycle/reset/render-audit smoke checks, the new
fail-closed feature-profile smoke test, and all ten MoltenVK configuration
modes. Its prepared official-1.4.1 artifacts have SHA-256:

```text
proxy:     d1cdd342593d8fab2013eec89916142841230d1d505ffdb6fb0db7655a3c088f
MoltenVK:  d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

Fresh replacement and embedded Vulkan/surface probes, the 24-cycle reset
composite on both 1.4.1 and 1.4.2, and the swizzled/identity swapchain-view
boundary probe all pass on the Apple M4. No Steam, launcher, or ESO process
was started.

## Preflight

- Exact source candidate committed as `26d26ac`.
- Python tests and compilation, all shell syntax checks, whitespace checks,
  warnings-as-errors compilation, and Clang static analysis passed.
- Re-run `scripts/check-update.sh` and require the selected ESO fingerprint.
- Confirm ESO, Steam, and the launcher are stopped.
- Recheck the current Experiment 0017 install, pristine restore source, target
  fingerprint, both pipeline caches, settings, prepared hashes, and restore
  path.
- Receive explicit approval for this new game-bundle modification.
- Preserve both caches and settings, restore the original loader, rebuild from
  that real loader, require byte-identical artifacts, then install only
  `legacy-feature-profile`.
- Prepare the ignored evidence boundary and automatically verify installed
  hashes and marker before requesting a user run.

## User procedure

This is a repair validation, not a diagnostic telemetry run:

1. launch once through the normal Steam path and enter the existing world;
2. change fullscreen resolution once;
3. report only whether rendering remains normal, becomes black/solid, freezes,
   or crashes, then exit.

No travel, HUD, screen capture, FPS recording, extra option change, or extended
play is requested. If the known low-performance state unexpectedly appears,
do not immediately run again; analyze the collected run first.

## Pass/fail

- **Pass:** normal-performance world rendering remains correct after the one
  resolution reset.
- **Fail:** black, solid-color, or frozen output recurs.
- **Inconclusive:** the target, mode, exact 18/18 mask, create-device
  validation, or requested reset is not observed, or fixed evidence coverage
  fails.

## Result

The source candidate is fixed at commit `26d26ac`. At approximately
2026-07-26 00:46 KST, after explicit user approval, Experiment 0017 was
restored to the checked pristine loader with both pipeline caches preserved.
The active loader then matched the pristine source, the marker was absent, and
the ESO 12.0.7 target fingerprint remained current.

The bridge was rebuilt from that real original loader. Its proxy and official
MoltenVK 1.4.1 hashes exactly reproduced the prepared values. Installation in
`legacy-feature-profile` mode preserved both caches and `UserSettings.txt`.
Post-install verification confirmed:

- ESO, Steam, and the launcher remained stopped;
- target SHA-256, UUID, client version, and databuild were current;
- the installed proxy and runtime were byte-identical to the built artifacts;
- the marker contained exactly `legacy-feature-profile`;
- both pipeline-cache hashes and the settings hash were unchanged;
- the quick update gate returned `READY`.

The ignored evidence boundary is
`artifacts/experiment-0018-20260725T154658Z`.

The user launched once through Steam, entered the world, changed fullscreen
resolution once from 1920 x 1200 to 2048 x 1280, and reported solid-color
output. No repeat was requested. All 48 evidence checksums verify. The exact
selected run is `20260725T154920.411050000Z-pid85904`; startup passed with two
36-to-18 feature queries and one device creation enabling exactly 18 features
with zero prohibited fields. The settings comparison has exact structural
identity and only the two requested resolution dimensions changed.

Three swapchain generations completed without lifecycle anomalies. Generation
3 continued for 313 acquire/present pairs before orderly shutdown. Every one
returned `VK_SUBOPTIMAL_KHR`. Because the observation wrapper logs every
non-success result, it consequently took its mutexes, formatted, and flushed
626 hot-path records after the reset instead of stopping after the nominal
eight-frame prefix.

## Rollback

Experiment 0018 is installed. The pristine Bink loader, both pipeline caches,
settings, displaced Experiment 0017 marker, and prior evidence remain present.
The checked restore path can return the active proxy to the pristine loader
without replacing either cache.

## Follow-up

The newly enabled core-feature category is excluded. Before returning to
device-property or internal Metal state analysis, remove the diagnostic
lifecycle wrappers from the performance path. The observed persistent
suboptimal result turns their nominally bounded acquire/present logging into
per-frame mutex, formatting, and file-I/O work. This is a valid performance
cleanup and a narrow timing counterfactual, not yet a demonstrated rendering
repair.
