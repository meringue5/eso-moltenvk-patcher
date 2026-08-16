# Experiment 0036: runtime compiler-readiness gate

- Date: 2026-08-17
- Outcome: **running; non-game validation succeeded and ESO validation is pending**
- Rollback: **available and verified; candidate remains installed for validation**

## Question

Can the bridge require a real Metal compiler-service round trip before ESO
begins its own pipeline initialization, so a process cannot silently continue
through the compiler-service-absent state seen in Experiment 0035?

## Hypothesis

A cache-independent compute-pipeline canary created immediately after the first
`VkDevice` should force Metal library and pipeline compilation. Making the
canary shader unique per process should prevent the system compiler cache from
bypassing that round trip. A failed canary must destroy every temporary Vulkan
object and the newly created device before returning an error to ESO.

## Target and change set

- Exact ESO 12.0.8 target and official MoltenVK 1.4.2 remain unchanged.
- The bridge's `vkCreateDevice` compatibility wrapper creates one private
  compute shader, descriptor-set layout, pipeline layout, and compute pipeline
  with `VK_NULL_HANDLE` as the Vulkan pipeline cache.
- A process/time nonce is embedded in a storage-buffer write in the SPIR-V
  before shader-module creation. The resulting shader source is unique for
  every process and cannot reuse the preceding process's identical Metal
  library artifact.
- The canary objects are destroyed before `vkCreateDevice` returns. On failure,
  the bridge also destroys the new `VkDevice`, clears the returned handle, and
  propagates the error.
- No game setting, user cache, ESO executable, or packaged runtime was changed.
  The rebuilt bridge loader is the only installed candidate payload.

## Preflight

`scripts/check-update.sh` reported `CURRENT` for ESO 12.0.8, and
`scripts/status.sh` initially reported the public 0.1.1 bridge and official
1.4.2 runtime installed with a verified restore path. The candidate was rebuilt
from source before any install operation.

## Procedure

1. Compared the bad and smooth Experiment 0035 process windows for Rosetta
   analysis activity.
2. Repeated the existing startup-surface probe with the production asynchronous
   submit/concurrent-compile settings and with conservative synchronous/default
   compilation settings.
3. Added the initial cache-independent compute-pipeline canary and verified it
   against the real official 1.4.2 runtime and Apple M4 Metal device.
4. Observed that a fixed canary could itself become a system compiler-cache hit,
   then changed the SPIR-V storage write to contain a process-unique nonce.
5. Ran the unique canary five times under the exact production MoltenVK
   configuration and inspected the corresponding unified-log compiler events.
6. Exercised success, pipeline-failure, and missing-function paths with fake
   Vulkan functions in the normal build smoke test.
7. After the exact target, verified restore, build, non-game, and shared
   bundle-idle gates passed, restored the pristine loader and installed the
   candidate in cache-preserving mode.

The agent did not launch ESO, Steam, or the ZeniMax launcher.

## Evidence

The bad and smooth Experiment 0035 windows each recorded 18 completed Rosetta
analyses and no `oahd` event. Rosetta activity therefore does not distinguish
that pair.

Five existing startup-pipeline trials under the production configuration and
five under the conservative configuration all passed. This does not reproduce
the ESO-only failure and supplies no evidence that disabling asynchronous
submission or concurrent compilation would correct it.

The first fixed canary created ten `MTLCompilerService` connections and two
successful jobs: one `MTLBuildLibraryFromSourceToArchive` and one
`MTLBuildFunctions`. Subsequent identical canaries could use the system cache,
so the fixed shader was rejected as an insufficient readiness guarantee.

The process-unique canary produced these five direct probe durations:

```text
46.405 ms
36.059 ms
35.699 ms
36.276 ms
36.760 ms
```

All five returned `VK_SUCCESS`. The unified log independently recorded five
distinct probe processes, 50 compiler-service connection events, ten
compilation starts, ten matching successes, and zero failures. Each process
performed exactly one library build and one function/pipeline build.

The complete source build passed. Its compatibility smoke test also confirmed:

- success destroys the temporary pipeline, pipeline layout, descriptor-set
  layout, and shader module while retaining the new device;
- pipeline failure destroys all previously created temporary objects and the
  device; and
- a missing required device function creates no temporary object and destroys
  the device.

## Result

The candidate establishes a deterministic, in-process compiler-service
readiness invariant in the non-game environment. Every tested process had to
complete a new Metal library and pipeline compilation before the Vulkan device
was exposed to the caller. The fixed-shader cache loophole was found and closed
before game installation.

Post-install status reports the exact target, current official runtime, current
marker, and installed bridge. The installed loader is byte-identical to the
rebuilt candidate with SHA-256
`ae8c7155ae8277fb67be971f7aab9421c0dfc27f39b0880dae9b48b62f4f7c22`.
The active pipeline cache retained its pre-install SHA-256
`7ad326880e6e46e78fb7e88e2d55c2c0153af6d85a0a718af141cfe41e3c2542`.

## Interpretation

**Confirmed:** Rosetta analysis is not the observed bad/good discriminator, and
a simple switch to conservative queue/compiler settings has no supporting
non-game evidence.

**Confirmed:** the unique canary forces the same compiler-service connection
class that was missing from the bad Experiment 0035 process. It is independent
of ESO's pipeline cache and does not distribute or delete user cache data.

**Candidate behavior:** if compiler-service non-engagement causes the pink and
low-FPS state, the canary should either initialize the service before ESO's
pipelines or fail the Vulkan-device creation cleanly instead of allowing the
degraded session.

**Unproven:** non-game success does not establish that compiler-service absence
is the cause rather than a consequence of ESO's bad state. Only a
user-controlled normal-path run with the candidate installed can establish
whether the first ESO process becomes consistently smooth.

## Rollback

The pristine loader was restored and independently verified before installing
the candidate. The candidate now remains installed for the pending validation;
the same pristine restore path is still available. User settings and every
pipeline-cache generation were preserved in place.

## Follow-up

Request one user-controlled normal launch. Pass requires
`RUNTIME_READINESS: compiler_canary=pass` before ESO's
own startup milestones, no pink surface, and normal frame pacing on the first
process. Any pink/low-FPS result despite a passed canary falsifies the current
causal hypothesis and blocks release.
