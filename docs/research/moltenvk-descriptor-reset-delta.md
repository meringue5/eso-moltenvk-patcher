# MoltenVK descriptor behavior at the live-reset boundary

- Review date: 2026-07-25
- Compared tags: official `v1.0.18` and `v1.4.1`
- Local trigger: Experiment 0012 resolution reset

## Confirmed upstream delta

MoltenVK 1.4.1 introduced a new descriptor state tracker and descriptor
set/pool implementation. Its
[release notes](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/Whats_New.md)
warn that the implementation is less forgiving when applications bind
descriptors whose targets have been destroyed. Its
[configuration reference](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/MoltenVK_Configuration_Parameters.md)
defines `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1` as treating all descriptors as
partially bound.

Source inspection at tag `v1.4.1`, commit
`db445ff2042d9ce348c439ad8451112f354b8d2a`, shows that the discrete-descriptor
pipeline emits `BindTextureWithLiveCheck`, `BindBufferWithLiveCheck`, and
related operations under that setting. At command encoding, a non-null
resource that fails the live-set lookup is not bound. The setting avoids
dereferencing a destroyed Metal object; it does not reconstruct the
application's intended replacement resource.

Source inspection at tag `v1.0.18`, commit
`a27de2054692a708c0f8d34dff77fdf5a8ea5a5e`, shows the older implementation
materializing `MTLTexture`, `MTLBuffer`, offsets, and samplers into
`MVKDescriptorBinding` during `vkUpdateDescriptorSets`, then binding those
arrays directly. It has no live-resource configuration or the 1.4.1 descriptor
state tracker.

## Local evidence joined to the delta

Experiment 0012's first eight frames after live reset:

- destroyed 77 image views and created 67;
- allocated 456 descriptor sets;
- made 93,707 calls to `vkUpdateDescriptorSets`;
- bound descriptor sets 7,699 times and issued 7,699 indexed draws;
- submitted all eight command buffers with no Vulkan failure;
- still presented persistent solid-color output.

This does not prove that ESO submitted an invalid Vulkan descriptor. It does
show that the observed symptom lies in a boundary whose implementation changed
substantially between the embedded and replacement runtimes, and that the
enabled compatibility option can suppress a dead binding without repairing its
contents.

## Next discriminant

The failed reset allocated or freed no Vulkan command buffers inside the trace
window. MoltenVK 1.4.1 documents command pooling as reusing command memory,
whereas disabling it allocates and destroys that memory for each execution.
The next narrow counterfactual is therefore command pooling disabled with every
other compatibility value unchanged. A rendering pass would implicate reused
command/resource state; another failure would move the investigation to
descriptor content/lifetime tracking rather than further swapchain or memory
allocation changes.
