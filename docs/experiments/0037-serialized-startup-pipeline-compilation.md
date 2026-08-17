# Experiment 0037: serialized startup pipeline compilation

- Date: 2026-08-17
- Outcome: **running; source and non-game gates passed**
- Rollback: **available and verified; installation pending**

## Question

Does MoltenVK's optional maximum-concurrent-compilation mode intermittently
leave ESO in the slow renderer-initialization path, and can restoring the
MoltenVK default compilation policy make startup reliable without changing
queue submission, caches, settings, or the compositor neutralizer?

## New evidence selecting the hypothesis

The user reported another pink/low-FPS start on the first play after the Mac
had remained in clamshell sleep overnight. The exact process was
`20260817T025220.324814000Z-pid67855`. It began about 41 minutes after the
recorded full wake, so a simple "Metal was not ready immediately after wake"
explanation is not supported.

The bridge loaded official MoltenVK 1.4.2, activated all 17 redirects, and
again neutralized 79 exact compositor draws before forwarding at generation 2
ordinal 150. This excludes total bridge nonactivation but does not prove that
the visible pink interval came from the already-neutralized compositor window.

The stronger discriminator is ESO's own interface state sequence:

- the low process entered `WaitForGameDataLoaded` and
  `WaitForCharacterDataLoaded`, then took about 13.0 seconds from character
  selection to `RENDERER Complete`;
- the preceding completed normal process advanced directly from account login
  to character selection and reached `RENDERER Complete` in about 2.7 seconds;
- both processes completed the Experiment 0036 compiler canary, and the normal
  process remained canary-only for more than nine minutes.

Confirmed: the low state is an alternate/delayed ESO renderer-initialization
path, not merely an unresponsive compiler service. Inference: the only
non-default compilation policy in the production profile,
`MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1`, is now a bounded causal
candidate. MoltenVK documents it as an optional way to maximize concurrently
executing compilation tasks; its default is disabled. This experiment does not
claim that concurrency is the cause before an ESO A/B result exists.

## Candidate

For only the installed `startup-compositor-neutralize` profile:

- set `MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=0`;
- retain asynchronous queue submission, argument buffers disabled, live
  resource checks disabled, MTLHeap where safe, command pooling, and the exact
  71--149 compositor-neutralization window;
- remove the failed Experiment 0036 device-readiness canary from production
  device creation;
- wrap the first 64 `vkCreateGraphicsPipelines` calls with process-monotonic
  begin/end records containing call ID, requested pipeline and shader-stage
  counts, cache presence, result, non-null output count, and duration; and
- forward every original argument and result unchanged. The timing wrapper
  adds no wait, cache mutation, pipeline creation, or synchronization.

The single repair variable is maximum concurrent compilation. Removing the
failed canary restores the pre-Experiment-0036 device path; the bounded timing
records are diagnostic observation only.

## Safety and pass criteria

- Exact target remains ESO 12.0.8, databuild `3288357`, executable SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- Runtime remains official MoltenVK 1.4.2 SHA-256
  `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
- Restore/install must pass the shared bundle-idle gate and preserve settings,
  the active 1.4.2 cache, the pre-bridge backup, and the historical cache.
- Every retained pipeline BEGIN record must have a matching END record. Any
  Vulkan failure, missing return, bridge error, or configuration mismatch
  fails the candidate.
- One normal launch is useful evidence but cannot establish reliability.
  Repeated naturally occurring first launches without pink/low FPS support the
  hypothesis; recurrence with the new configuration falsifies it as a repair.

## Non-game evidence

- Complete bridge build with warnings as errors: pass.
- Bink re-export and Rosetta self-patch probes: pass.
- Lifecycle probe: bounded pipeline begin/end records bracket the unchanged
  downstream call with exact count, result, and output assertions: pass.
- HDR/device compatibility and readiness cleanup controls: pass.
- Python suite: 133 tests pass.
- Python compile, shell syntax, and `git diff --check`: pass.
- Disposable release transaction fixture, including update recovery and
  uninstall/reinstall: pass.
- Prepared proxy SHA-256:
  `79757b5ddeba018d2b93c4078af5e635097bf04e6d651a86495a6b6c5043ee5d`.

No game, launcher, or Steam process was launched by the agent. No game-bundle
file, setting, or cache was changed while preparing this source candidate.

## Procedure

1. Recheck the exact update target, pristine recovery path, source build, and
   shared bundle-idle gate.
2. Preserve current cache and settings identities, restore the pristine loader,
   and install this candidate under the standing cache-preserving authorization.
3. Verify the installed proxy and runtime hashes, marker, target attestation,
   recovery state, and unchanged cache/settings identities.
4. The user performs one ordinary Steam-authenticated launch. The agent does
   not launch ESO, Steam, or the ZeniMax launcher.
5. The user reports pink/no-pink and low/normal FPS. The agent correlates that
   observation with pipeline call/return timing and ESO's renderer-completion
   path.

## Result

Source and non-game gates passed. Installation and user validation are pending.
