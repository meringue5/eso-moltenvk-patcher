# Experiment 0037: serialized startup pipeline compilation

- Date: 2026-08-17
- Outcome: **failed as a reliability fix; diagnostic timing succeeded**
- Rollback: **available and verified; rollback pending Experiment 0038 transaction**

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

Source and non-game gates passed from committed source `30b499a`. The shared
bundle-idle gate passed with idle Steam open and no ESO file or update activity.
The Experiment 0036 candidate was restored to the pristine loader and this
candidate was installed with all caches preserved in place.

The installed proxy matches the prepared source build byte for byte at
`79757b5ddeba018d2b93c4078af5e635097bf04e6d651a86495a6b6c5043ee5d`.
The installed official MoltenVK remains `aef00b13...`; the exact ESO target,
attestation, enable marker, restore source, and three cache identities pass.
Installation preserved:

```text
settings:             9dd0e4234bb1f0a447b0c870b8c6faa072afbabb729775702aa1dd527b07d5b6
active 1.4.2 cache:   db4f08a9a092c7586cde9b8068eb24e2de8c80704f66eb761495116d20306f2b
old embedded backup: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
1.4.1 backup:        5869aa929521788681e665e803e5876487a4eb8cede9589f56c48e05087c404b
```

No Steam, launcher, or ESO process was launched by the agent. User validation
is pending.

## First ESO validation amendment: normal extended play

The user performed an ordinary authenticated launch and reported normal,
extended play. Exact bridge run
`20260817T055527.350588000Z-pid73931` verified the candidate configuration:

```text
maximize_concurrent_compilation=0
readiness_canary=disabled
pipeline timing retained=64 calls
pipeline begins/ends=64/64
pipeline results=64 VK_SUCCESS, 64 non-null outputs
maximum retained call duration=1.783 ms
compositor suppressions=79
compositor forward latch=generation 2 ordinal 150
```

ESO followed the same early path as the preceding normal process: it advanced
directly from `AccountLogin` to `CharacterSelect`, then recorded
`RENDERER Complete` about 2.77 seconds later. The interface log contains
continued world/zone activity for more than three hours after process start.
The user did not separately classify whether any pink frame was visible in
this report, so only normal FPS and extended play are confirmed visually.

This run is a direct counterexample to the startup compositor neutralizer being
sufficient to cause low FPS: the exact 79-draw suppression and ordinal-150
forwarding mechanism remained active throughout startup, yet the resulting
session was normal and long. It does not prove that the neutralizer can never
interact with another startup state. A clean future A/B, if needed, must retain
non-maximized compilation and disable only neutralization; the existing
`performance-aggressive` mode is not that control because it also re-enables
maximum concurrent compilation.

After process exit, ignored evidence preserved the complete logs and the
naturally updated caches. The active pipeline cache SHA-256 was
`1b2cbb651e4faf94f79e37fa4d1b59a6679368ade7a1bc1734fe16f58d176914`;
`ShaderCache.cooked` was
`dbda194202ea64f19b743ac6d00c86fb84448940cbd1909314078c91e7b0b3a2`.
No live cache or setting was replaced.

## Recurrence amendment: non-maximized compilation falsified as a repair

After several days of apparently normal launches, the user reported visible
pink and low FPS in run `20260825T141444.855727000Z-pid71293`. The exact
Experiment 0037 bridge was active: MoltenVK 1.4.2 loaded, all 17 redirects
activated, non-maximized compilation verified, 79 draws were suppressed, and
the forward latch occurred at generation 2 ordinal 150.

All 64 retained graphics-pipeline calls returned `VK_SUCCESS` with non-null
outputs, and the slowest retained call took only 6.775 ms. The discriminator
was before those calls: the bulk wave beginning with call 5 was not issued
until about 32.60 seconds after the timing origin, versus about 11.76 seconds
in the first normal run. ESO delayed `RENDERER Complete` to about 13.86 seconds
after character selection, again matching the alternate path.

This recurrence falsifies maximum-concurrent-compilation disablement as a
reliable repair. The diagnostic succeeded by showing that ESO, rather than a
slow or failed MoltenVK pipeline call, delayed entry into the bulk graphics
pipeline path. Experiment 0038 is the planned single-variable control: retain
the Experiment 0037 compilation and timing configuration while disabling only
the cosmetic compositor neutralizer and its audit machinery.
