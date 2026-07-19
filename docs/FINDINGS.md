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
ESO return offset observed in Experiment 0001.

MoltenVK 1.0.18 does not advertise `VK_EXT_hdr_metadata` and returns NULL for
that proc. MoltenVK 1.4.1 advertises the extension; GIPA returns non-null, while
GDPA returns NULL until the extension is enabled on the device and non-null
after it is enabled. Proc compatibility tests must therefore reproduce both the
advertised extension set and the extensions enabled at device creation. The
evidence strongly suggests an HDR extension-negotiation mismatch, but does not
yet prove which null pointer ESO called.
