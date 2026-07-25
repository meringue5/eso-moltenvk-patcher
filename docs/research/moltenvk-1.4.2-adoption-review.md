# MoltenVK 1.4.2 adoption review

- Review date: 2026-07-26
- Scope: official v1.4.2 release, Apple M4, x86_64 ESO through Rosetta, and
  Experiment 0020's exact `performance-aggressive` configuration
- Decision: recommend a separately prepared 1.4.2 maintenance upgrade; do not
  claim that it fixes ESO's loaded-world graphics reset

## Official release identity

KhronosGroup published
[MoltenVK v1.4.2](https://github.com/KhronosGroup/MoltenVK/releases/tag/v1.4.2)
as a non-prerelease release from tag commit
[`db66022459ffb663aa2b50f6b018bc2e124f5edf`](https://github.com/KhronosGroup/MoltenVK/commit/db66022459ffb663aa2b50f6b018bc2e124f5edf).
GitHub's release metadata reports:

```text
release published: 2026-07-24T14:00:46Z
MoltenVK-macos.tar:
  size:   59,563,008 bytes
  SHA256: f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e
```

The downloaded archive matches that SHA-256. Its official dynamic macOS
library is a universal x86_64/arm64 binary with SHA-256:

```text
aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

Only the x86_64 slice is used by the current ESO process. No release file is
committed to this repository.

## Fix applicability

The release contains many fixes, but their relation to ESO is not uniform.

### Potentially useful

- [`9c66f04`](https://github.com/KhronosGroup/MoltenVK/commit/9c66f04e8afd6dd4729d8d1543a2f10b75de2292)
  encodes Metal barriers for Vulkan subpass dependencies. The prior ESO audit
  checked public barriers and attachment state but did not retain render-pass
  dependency counts. This fix is below that public-state boundary and cannot
  be excluded from ESO's full render graph without another game trace.
- Primitive-restart state desynchronization, the inverted dynamic
  `MVKBitArray` query, variable-descriptor-count querying, pixel-format null
  handling, and several SPIRV-Cross corrections are general correctness fixes.
  None is proven to cause the reset symptom, but retaining the older defects
  has no compatibility benefit once the new runtime passes the bridge gates.

### Known narrow or presently inactive

- [`9a5e233`](https://github.com/KhronosGroup/MoltenVK/commit/9a5e233ef08e3a0f58c7b90053385cfb5cacde68)
  fixes a cached derived Metal texture behind a swapchain image view.
  Experiment 0017 proved the bug and repair, but ESO's captured identity views
  bypass that cache branch.
- [`595b0b2`](https://github.com/KhronosGroup/MoltenVK/commit/595b0b2f17b6dced5b6bb7bfa933d892a3cb737a)
  prevents Apple Silicon channel corruption only when one image has both
  `COLOR_ATTACHMENT` and `TRANSFER_SRC` usage. All 144 reset-created-image
  records in Experiment 0016's accumulated preserved log used `0x14`
  (`SAMPLED | COLOR_ATTACHMENT`) or `0x20`
  (`DEPTH_STENCIL_ATTACHMENT`), not `TRANSFER_SRC`.
- The 1 x 1 swapchain-recreation correction does not match the captured
  replacement swapchains, whose extents were nonzero and exact.
- Argument-buffer alignment/residency and imported-Metal-resource fixes are
  inactive in the established profile: Metal argument buffers are disabled,
  and ESO does not enable the relevant external-memory extension path.
- AMD, Intel, NVIDIA/Mac1, and Apple10/macOS 26 capability changes do not
  describe the tested Apple M4 path.

This classification means 1.4.2 is not an evidence-backed repair for the
solid-output reset. It does contain applicable general correctness work and
one lower-level render-pass synchronization fix that the existing audit could
not exclude.

## Exact non-game verification

The official release binary was built into the bridge from a shadow app using
the checked pristine loader. The resulting proxy retained the Experiment 0020
SHA-256 because the replacement runtime is loaded dynamically:

```text
proxy:     5019d4eb552f89ea59bfda9d38e2f2c98ce36f2490c7c41294750c62ba68acde
MoltenVK:  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

The complete source build passed Bink symbol re-export, Rosetta self-patching,
warnings-as-errors compilation, all twelve configuration modes, and the
existing compatibility smoke probes. The exact 0020 profile was verified:

```text
LIVE_CHECK_ALL_RESOURCES=0
USE_METAL_ARGUMENT_BUFFERS=0
USE_MTLHEAP=1
SYNCHRONOUS_QUEUE_SUBMITS=0
SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1
USE_COMMAND_POOLING=1
PREFILL_METAL_COMMAND_BUFFERS=0
```

On the Apple M4, the official release also passed:

- all 24 alternating 2048 x 1280 / 1920 x 1200 reset-composite cycles;
- long-lived descriptor-set updates and alternating command-buffer/pool reset;
- asynchronous queue submit and concurrent compilation;
- exact Metal pixel validation;
- ESO-era device creation and all 100 proc routes;
- HDR device-extension filtering, with exactly one removal;
- HDR surface-format filtering, from 60 raw to 59 visible formats.

A balanced descriptor-encode comparison used four processes and 28 timed
samples per version, 100,000 draws per sample, live checking disabled, and
alternating version order:

```text
1.4.1 aggregate median: 176.068 ns/draw
1.4.2 aggregate median: 180.105 ns/draw
difference:              1.4.2 is 2.293% slower
```

The process medians overlap, and the difference is below the existing 3%
meaningful-change threshold. The result is no measured performance benefit
and no material descriptor-encode regression; it is not an ESO FPS result.
Raw outputs remain outside Git under
`/private/tmp/teso4m4-mvk142-balanced`.

## Pipeline-cache transition

The active ESO cache begins with MoltenVK 1.4.1 pipeline-cache UUID prefix
`db445ff2`. Official 1.4.2 reports prefix `db660224`. The caches are therefore
not interchangeable.

An upgrade must preserve the current 1.4.1 cache under a versioned backup name
and start a separate 1.4.2 cache. Passing the 1.4.1 blob to 1.4.2 and allowing
ESO to overwrite the same file would destroy a useful rollback state and
confound cold-cache behavior. The pre-1.4.1 cache backup must also remain
untouched.

The first 1.4.2 launch will consequently incur a cold pipeline-cache start.
That is a temporary transition cost, not evidence of a steady-state
performance regression.

## Recommendation

Prepare the next runtime candidate with the official v1.4.2 archive and the
unchanged Experiment 0020 performance profile. Keep shader-source compression
out of that candidate so a runtime upgrade is not combined with a separate
memory/latency trade.

This recommendation is a maintenance decision:

- 1.4.2 removes known 1.4.1 defects;
- the exact bridge and M4 non-game gates pass;
- no material descriptor-path regression was measured;
- the only concrete operational cost is a separately managed cold cache.

It is not a promise that the loaded-world graphics reset will be repaired.
Installation still requires a new explicit approval and a fail-closed
cache-transition implementation.
