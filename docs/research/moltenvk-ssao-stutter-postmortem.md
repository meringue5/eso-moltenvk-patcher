# MoltenVK SSAO and stuttering post-mortem context

- Review date: 2026-07-19
- Scope: official MoltenVK documentation and issue tracker
- Local trigger: Experiment 0006 live switch from disabled ambient occlusion to
  SSAO, followed by solid-color presentation

## Local evidence boundary

Experiment 0006 establishes a precise local sequence: visually correct world
rendering, a user-controlled SSAO setting change, `DeviceWaitIdle`, swapchain
recreation, `OnDeviceReset`, renewed Metal compiler warnings, and then
solid-color frames with continued input response. It records no Vulkan error,
Metal command-buffer failure, device loss, or crash.

This does not reveal whether the fault is in the SSAO shaders, render-pass and
depth attachment state, the live reset, or the newly created pipeline cache
entries. macOS privacy-redacted the compiler-warning bodies.

## Upstream context

The official [MoltenVK 1.4.1 runtime guide](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/MoltenVK_Runtime_UserGuide.md#shader-loading-time)
states that runtime SPIR-V-to-MSL conversion is the slowest shader-loading step
and that serialized Vulkan pipeline caches can reduce shader-loading time and
runtime hiccups. Experiment 0006's warning burst and cache growth therefore
justify a warm-cache comparison; they do not prove the source of each stutter.

The official [MoltenVK 1.4.1 change history](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.1/Docs/Whats_New.md#moltenvk-141)
lists occlusion-query improvements and a fix for improper dynamic depth/stencil
attachment use. Vulkan occlusion queries are not screen-space ambient
occlusion, so the similar terminology is not causal evidence. The
depth/stencil fix is adjacent mechanism context only.

The long-running [MoltenVK subpass issue #490](https://github.com/KhronosGroup/MoltenVK/issues/490)
discusses depth and attachment reads across subpasses and Apple-GPU differences
in deferred rendering. It is not an ESO or SSAO symptom match and predates
MoltenVK 1.4.1 by years. It only shows that multi-pass depth/attachment
translation is a plausible class of compatibility boundary.

No exact public MoltenVK or ESO match for input-responsive changing solid-color
frames after a live SSAO toggle was found in the reviewed searches.

## Decision

Do not change another MoltenVK configuration variable yet. First restore the
known SSAO-off user setting and repeat the existing build with its warm cache.
For a later SSAO comparison, add narrow diagnostics around graphics-pipeline
creation and ESO's device-reset interval, then distinguish clean-start SSAO
from a live toggle. A successful clean start with SSAO would implicate the live
reset more strongly; failure in both cases would implicate the SSAO render path
more strongly.
