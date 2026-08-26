# Experiment 0038: no-neutralizer startup control

- Date: 2026-08-25
- Outcome: **succeeded as the performance-first control; recurrence monitoring remains**
- Rollback: **available and verified; control currently installed**

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

Committed source `8f59bac` passed the exact-target and shared bundle-idle gates.
Idle Steam was open, but no ESO, launcher, file-holder, or update activity was
present. Experiment 0037 was restored to the pristine loader, verified
inactive, and this control was installed without launching any application.

The installed bridge is byte-identical to the prepared build at
`837ce644b3fa133ae8a5322fff8c0947d6fb8ccfc8c8d205f20c230b746d1580`.
The marker records `startup-pipeline-timing-control`; official MoltenVK remains
`aef00b13...`; the exact ESO attestation and all three pipeline-cache identity
headers pass. The restore/install transaction preserved these hashes:

```text
settings:             104e894803e70dae30fdab887474a8f3116387375614484d36c3755c58745fb0
active 1.4.2 cache:   896a9326fa71733119b1ec2a6a8fe74beaa7a53aa5526aad7907713921647684
old embedded backup: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
1.4.1 backup:        5869aa929521788681e665e803e5876487a4eb8cede9589f56c48e05087c404b
shader cache:        055a55c821b5dfeb8db4d1f7d290ce8003a46924449d1a190afcde5a19822f6c
```

User-controlled validation is pending. Pink is expected because the cosmetic
neutralizer is deliberately absent; the report needed is normal versus low
FPS. No forced repetition or launcher restart is required.

## First user-controlled validation: normal FPS with pink preserved

The user launched through the normal authenticated path and reported the
intended separation: the pink startup surface remained visible, while the
low-FPS mode did not occur. Exact bridge run
`20260825T142936.783874000Z-pid77809` confirms that the result came from this
control rather than an inactive bridge:

```text
mode:                               startup-pipeline-timing-control
compositor neutralizer records:     0
pipeline timing begins/ends:        64 / 64
pipeline results:                   64 VK_SUCCESS, non-null output each
maximum retained call duration:     1.547 ms
bulk wave beginning at call 5:     11.652 s
CharacterSelect -> RENDERER Complete: 2.685 s
visual result:                      pink present, FPS normal
```

The preceding low-FPS Experiment 0037 run began its bulk wave at 32.603
seconds and completed the renderer 13.86 seconds after character selection.
The control therefore returned to the previously observed normal startup path
while leaving ESO's original pink placeholder visible.

The active pipeline cache from the low run was preserved unchanged into this
launch at SHA-256 `896a9326...`; no cache reset or replacement was used to
obtain the normal result. The successful launch naturally rewrote it to
`430044ec230504d438b8d41d4bc1559953aeab3220d78d57a846fc066292e4fb`.
`ShaderCache.cooked` remained
`055a55c821b5dfeb8db4d1f7d290ce8003a46924449d1a190afcde5a19822f6c`.
This weakens a permanently poisoned cache as a sufficient explanation.

Interpretation is deliberately bounded. Pink is not the cause of low FPS, and
slow or failed MoltenVK pipeline compilation is excluded for this pair. The
leading trigger is now the removed startup-neutralization subsystem: either
the 79 draw-to-clear substitutions or the lifecycle/input/draw tracking needed
to select them perturbed an existing ESO renderer-initialization race. A fresh
launcher lifetime and the fault's intermittency remain confounders, so one
normal control does not prove which part of that subsystem was causal.

The user explicitly chose the performance-first product policy: ship this
exact control, accept the visible pink interval as a known cosmetic issue, and
keep its optional repair as future work rather than risking the low-FPS mode.

## 2026-08-27 amendment: low FPS recurred without the neutralizer

A naturally occurring 0.1.2 launch later reproduced both pink and low FPS in
the exact no-neutralizer control. Run
`20260826T173857.097445000Z-pid322` loaded the expected bridge and runtime,
redirected all 17 entry points, recorded `compositor_neutralize=disabled`, and
completed 64/64 retained graphics-pipeline calls successfully. Call 5 did not
begin until 32.698 seconds, while `CharacterSelect -> RENDERER Complete` took
13.762 seconds. This matches the earlier alternate path despite removal of the
neutralization subsystem.

The first normal result remains valid as an observation but is superseded as a
repair hypothesis. Neutralizer removal does not prevent low FPS, and the
neutralizer is no longer a leading causal candidate. See Experiment
[0040](0040-no-neutralizer-low-fps-recurrence.md).
