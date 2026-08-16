# Experiment 0036: runtime compiler-readiness gate

- Date: 2026-08-17
- Outcome: **failed as a cold-start fix; the readiness gate passed its compiler-service boundary but low FPS recurred**
- Rollback: **available and verified; failed candidate remains installed as the documented checkpoint**

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

## ESO validation amendment: 2026-08-17

The user completed two ordinary normal-launch-path sessions with the installed
candidate. The first was smooth; the next reproduced low FPS. No reinstall,
cache deletion, settings change, launcher bypass, or agent-controlled launch
occurred between them.

| Observation | Smooth session | Low-FPS session |
|---|---:|---:|
| Bridge run | `20260816T151737.132517000Z-pid52549` | `20260816T164017.660186000Z-pid53111` |
| Local start | 2026-08-17 00:17:37 KST | 2026-08-17 01:40:17 KST |
| User result | smooth long play | low FPS; exited after about 105 seconds |
| ESO-side compiler-service connections | 10 | 10 |
| Compilation begins / successes / failures | 35 / 35 / 0 | 2 / 2 / 0 |
| Immediate canary-signature jobs | 1 library + 1 pipeline | 1 library + 1 pipeline |
| Later ESO compilation jobs | 33 | 0 |
| Compositor result | 79 suppressed; forward at ordinal 150 | 79 suppressed; forward at ordinal 150 |

Both processes started after the candidate loader was installed and used its
exact SHA-256. In each process, ten `MTLCompilerService` connections appeared
about one second after process start, followed by one successful
`MTLBuildLibraryFromSourceToArchive` and one successful `MTLBuildFunctions`
job. That is the exact signature produced by the process-unique canary. The
low-FPS process then reached two logged device-reset completions and a server
connection, demonstrating that device creation and application startup
continued.

The smooth process later performed 33 additional successful compilation jobs,
beginning at 00:22:53 KST and continuing during long play. The low-FPS process
performed no compilation beyond the two immediate canary-signature jobs before
it exited at approximately 01:42:03 KST.

The production bridge log unexpectedly omitted the required
`RUNTIME_READINESS` success record for both processes even though the unified
log contains the canary's immediate one-library/one-pipeline compiler
signature. This is an observability defect in the candidate and independently
fails the explicit logging acceptance criterion. It does not weaken the user
result: low FPS recurred after the compiler service was demonstrably reachable
and completed both immediate jobs successfully.

The active pipeline cache was preserved. After the low-FPS process exited it
remained 8,344,388 bytes with SHA-256
`330040db99ebeb322f360b6dd0e851c469c739d331093a55348559dd8a0668ee`.
`ShaderCache.cooked` retained its prior modification time. No between-session
cache snapshot exists, so this pair does not assign causality to the cache
delta.

### Amended result

Experiment 0036 is **failed as a cold-start reliability fix**. Proving that the
compiler service can compile one independent pipeline before ESO continues is
not sufficient to make ESO enter its normal graphics-pipeline initialization
path. Compiler-service absence in the earlier bad process was a useful marker,
but compiler-service readiness is not the root cause established by this test.

The candidate must not be packaged or released. Additional repetition with
the same binary is unnecessary: the next diagnostic must time ESO's first
`vkCreateGraphicsPipelines` wave and correlate those calls with compiler
connections and the user-visible state. The verified pristine restore remains
available; the failed candidate remains installed only as the current
documented checkpoint.

## Second ESO validation amendment: consecutive low-FPS process

At 01:49:27 KST the user made another ordinary launch in order to play. This
was not an agent-requested experiment. Run
`20260816T164927.826393000Z-pid56332` again reproduced low FPS, and the user
exited after approximately 39 seconds.

The bridge again loaded the exact replacement runtime, activated all 17
redirects, and reached the same 79-suppression/ordinal-150 compositor latch.
The unified log again recorded ten ESO-side compiler-service connections and
exactly two successful jobs with no failure: one library build and one
pipeline build at 01:49:29 KST. No later ESO compilation occurred. This is the
same canary-only signature as the preceding low-FPS process, PID 53111.

The consecutive failures establish that one restart does not deterministically
repair the condition. The user's longer-term recovery observation is that
restarting ESO, and sometimes the launcher, repeatedly eventually produces a
smooth process. That pattern is consistent with either a probabilistic startup
ordering issue or state evolving across process exits; it does not yet choose
between them.

There is direct evidence that state evolves even across failed processes. The
active pipeline cache after PID 53111 was 8,344,388 bytes with SHA-256
`330040db99ebeb322f360b6dd0e851c469c739d331093a55348559dd8a0668ee`.
After PID 56332 exited, the same-size file had SHA-256
`9828ac54bcb6d0e00c20e7eb2155cf9b707e8c58016d617a3399b39f70b4b48d`.
`ShaderCache.cooked` remained unchanged. The second cache generation and raw
logs were copied into ignored evidence before another launch could overwrite
them; the live files were not modified.

This cache rewrite is a new causal candidate, not proof. MoltenVK may serialize
nondeterministic or non-causal state at normal shutdown. The discriminating
test is to preserve the active cache after every user-initiated failed attempt
and immediately before or after the first eventual smooth attempt, without
deleting, replacing, or distributing it. Graphics-pipeline creation timing is
still required to determine what the changing state affects.

## Third ESO validation amendment: three failures and one recovery

The user then reported a bounded sequence of three consecutive pink/low-FPS
starts while leaving the ZeniMax launcher open, followed by one normal start
after closing that launcher and reopening it through Steam:

| User result | Bridge run | Compiler result during the observed startup |
|---|---|---|
| pink / low FPS | `20260816T165543.783831000Z-pid57578` | canary-only: 2 begins, 2 successes, 0 failures |
| pink / low FPS | `20260816T165624.961119000Z-pid57581` | canary-only: 2 begins, 2 successes, 0 failures |
| pink / low FPS | `20260816T165705.673358000Z-pid57593` | canary-only: 2 begins, 2 successes, 0 failures |
| normal | `20260816T165757.237933000Z-pid57639` | canary plus 1 later opaque request: 3 begins, 3 successes, 0 failures |

Every process recorded ten compiler-service connections and the canary's one
library plus one pipeline job. The normal process alone recorded an additional
successful `MTLBuildOpaqueRequest` about eight seconds after process start.
This is the earliest direct discriminator in this four-run sequence. The log
does not identify that opaque request as a specific ESO graphics pipeline, so
that relationship remains to be established by Vulkan-side timing.

The restarted ZeniMax launcher process began at 01:57:50 KST, seven seconds
before the normal ESO process. Steam itself remained the same long-running
process. This establishes temporal correlation in this sequence, not launcher
causality. The user explicitly reports that launcher restart has not been
necessary for every prior recovery, so launcher lifetime remains a secondary
classification field rather than a critical variable or proposed fix.

The active pipeline cache was modified at 01:58:13 KST, after the normal
process and its opaque compiler request had already started. Its early-normal
SHA-256 was
`9e3362420c52344124a1cef75809ba17e8e8223d67bdc9055b81de3f7207fb63`;
a consistent ignored copy was preserved without modifying the live file. This
is not the normal process's input cache. The three intervening post-failure
cache generations were not captured, so the sequence still cannot separate
cache evolution from scheduling or launcher-associated state.

The strengthened invariant is independent of launcher lifetime: low-FPS
processes complete only the forced canary work, whereas smooth processes begin
additional Metal compiler work. The next diagnostic remains bounded timing of
ESO's first `vkCreateGraphicsPipelines` calls and their return path.
