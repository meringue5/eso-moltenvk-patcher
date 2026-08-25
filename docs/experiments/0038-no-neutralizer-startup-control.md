# Experiment 0038: no-neutralizer startup control

- Date: 2026-08-25
- Outcome: **running; source and non-game gates passed, installation pending**
- Rollback: **available; Experiment 0037 remains installed until the transaction gate**

## Question

Does the bounded startup compositor neutralizer contribute to ESO's
intermittent pink/low-FPS startup state, or does the alternate startup path
begin before that cosmetic repair can matter?

## Triggering recurrence

After several days and multiple apparently normal starts, the user reported
visible pink followed by low FPS in exact bridge run
`20260825T141444.855727000Z-pid71293`. The run loaded official MoltenVK 1.4.2,
activated all 17 redirects, verified the exact 12.0.8 executable, suppressed
the expected 79 compositor draws, and latched forward at generation 2 ordinal
150. Total bridge nonactivation is therefore excluded.

The retained graphics-pipeline timing directly falsifies Experiment 0037 as a
reliability repair:

```text
retained pipeline calls:              64 BEGIN / 64 END
results:                              64 VK_SUCCESS, non-null output each
maximum retained call duration:       6.775 ms
first call since timing origin:        9.726 s
bulk wave beginning at call 5:       32.603 s
normal Experiment 0037 call 5:       11.764 s
low-run delay before the bulk wave:  about 20.8 s
```

ESO entered `WaitForGameDataLoaded` and `WaitForCharacterDataLoaded`, reached
`CharacterSelect` at 23:15:03.577 KST, and did not record `RENDERER Complete`
until 23:15:17.441, about 13.86 seconds later. The first normal Experiment 0037
session completed the renderer about 2.77 seconds after character selection.

Confirmed: the retained MoltenVK pipeline calls themselves succeeded quickly.
ESO delayed issuing the bulk pipeline-creation wave. Inference: the alternate
startup path is selected upstream of actual graphics-pipeline compilation.
The neutralizer remains a causal candidate only because it is the remaining
intentional startup mutation; identical 79-draw suppression in normal and bad
runs means causality is not established.

Ignored local evidence preserves the complete triggering logs and both caches
under `artifacts/experiment-0038-low-before-neutralizer-control-20260825T231444KST/`.
The active pipeline-cache SHA-256 after the low run is
`896a9326fa71733119b1ec2a6a8fe74beaa7a53aa5526aad7907713921647684`;
`ShaderCache.cooked` is
`055a55c821b5dfeb8db4d1f7d290ce8003a46924449d1a190afcde5a19822f6c`.
No live cache or setting was replaced while preserving this evidence.

## Single-variable control

The new `startup-pipeline-timing-control` mode retains Experiment 0037's:

- official MoltenVK 1.4.2 and exact ESO compatibility redirects;
- disabled live-resource checks and Metal argument buffers;
- MTLHeap-where-safe, command pooling, and asynchronous queue submission;
- `MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=0`;
- disabled readiness canary; and
- bounded timing for the first 64 `vkCreateGraphicsPipelines` calls.

It disables only startup compositor neutralization and its supporting color,
draw, input, presentation, and compositor audits. The lifecycle router returns
every non-pipeline function unchanged. A visible pink interval is expected and
accepted in this control; FPS and the renderer path are the outcome variables.

## Safety and pass criteria

- Exact target remains ESO 12.0.8, databuild `3288357`, executable SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- Runtime remains official MoltenVK 1.4.2 SHA-256
  `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
- Restore/install must pass the shared bundle-idle gate and preserve all
  settings and cache identities.
- Any non-pipeline lifecycle wrapper, neutralizer/audit arm record, unmatched
  timing pair, Vulkan failure, or configuration mismatch fails the mechanism.
- A low-FPS recurrence excludes the neutralizer as the cause. Normal starts
  support the neutralizer-interaction hypothesis but one start cannot prove it
  because the fault is intermittent.

## Non-game evidence

- Complete bridge build with warnings as errors: pass.
- Bink re-export and Rosetta self-patch probes: pass.
- Lifecycle probe: only graphics-pipeline creation is wrapped, with one exact
  BEGIN/END pair and unchanged downstream result: pass.
- HDR/device compatibility and configuration probes: pass.
- Python suite: 134 tests pass.
- Python compile, shell syntax, and `git diff --check`: pass.
- Disposable release transaction fixture, including compatible-update
  recovery and install/remove/reinstall: pass.
- Prepared proxy SHA-256:
  `837ce644b3fa133ae8a5322fff8c0947d6fb8ccfc8c8d205f20c230b746d1580`.

No game, launcher, or Steam process was launched by the agent during source
preparation.

## Procedure

1. Recheck the exact update target, current installed state, restore path,
   source build, and shared bundle-idle gate.
2. Preserve current cache and settings identities, restore the pristine
   loader, and install the timing-only control under the standing
   cache-preserving authorization.
3. Verify installed proxy/runtime hashes, marker, attestation, restore state,
   and unchanged cache/settings identities.
4. The user performs an ordinary Steam-authenticated launch. A pink splash is
   expected; the user reports only whether FPS is normal or low.
5. Correlate that observation with pipeline timing and ESO renderer state. Do
   not require a launcher restart or forced repeated play.

## Result

Pending installation and user-controlled launch.
