# Experiment 0031: fixed-window startup compositor neutralization

- Date: 2026-08-02
- Outcome: **running; candidate installed**
- Rollback: **not started; pristine restore path checked**

## Question

Can the canonical-magenta startup interval be replaced with black by
withholding the exact final-compositor draw throughout its independently proven
startup window, without relying on the premature descriptor transition that
invalidated Experiment 0030?

## Evidence selecting the window

Experiment 0026 sampled generation-2 ordinal 70 as opaque black, ordinals 80
through 140 as exact `#FF00FF`, and ordinal 150 onward as normal scene content.
Experiment 0027 found zero draws through ordinal 70 and the same sole indexed
final-compositor draw in every magenta and later scene sample. Experiment 0030
then observed the first exact target draw at ordinal 71, but suppressed only
that draw before incorrectly latching to forwarding at ordinal 72.

The complete evidence therefore supports one small deterministic boundary on
this exact target: replace exact compositor draws from the observed first draw
at ordinal 71 through ordinal 149, then forward ordinal 150 and everything
later. The descriptor signature is retained for completeness logging but no
longer controls the transition.

## Candidate

The marker remains the isolated `startup-compositor-neutralize` maintenance
mode, but its begin record identifies `strategy=ordinal-window` and exact
source/build identity distinguishes it from Experiment 0030.

Suppression requires all of the following:

- generation 2 and next present ordinal 71 through 149;
- indexed pipeline signature `c43e4410d3b33fe7`;
- pipeline-layout signature `d175d2c1daed112d`;
- ordered set layouts `e3c2499a89df1706` and `d0edad262f8c4230`;
- complete expected descriptor classes and a live tracked framebuffer; and
- an available original `vkCmdClearAttachments` destination.

Each match replaces the indexed draw with an opaque-black full-frame clear.
Ordinal 150 forwards the application draw and permanently latches forwarding.
Unexpected non-indexed target use, incomplete state, 96 suppressions, identity
mismatch, or any missing dependency fails open. Pixel readback is disabled.
No setting, asset, shader, MoltenVK configuration, or cache policy changes.

## Why not pre-present pixel replacement

A pre-present exact-magenta detector would be more adaptive, but the existing
sampler requires `vkQueueWaitIdle`, Metal readback command submission, and CPU
completion before every inspected presentation. Applying it throughout the
startup interval would add synchronization and allocation work to dozens of
frames. The fixed window instead reuses the already proven command-recording
hook, adds no readback or queue wait, and is fail-closed to one exact ESO build,
pipeline, layout, descriptor shape, generation, and bounded interval.

## Non-game evidence

- Corrected Experiment 0030 analyzer: the live ordinal-71/72 trace now returns
  `INCONCLUSIVE` because suppression did not span the proven magenta interval.
- Synthetic lifecycle probe: an exact complete compositor draw is replaced by
  black; a changed descriptor state is also replaced rather than prematurely
  forwarded; incomplete target state forwards unchanged.
- Dedicated Experiment 0031 analyzer: requires suppressions spanning at least
  the ordinal-80-through-140 proven magenta samples, exact target identity,
  one ordinal-150 `present-deadline` latch, ordinal-180 finish, no readback, and
  no lifecycle error/truncation.
- Full bridge build and MoltenVK configuration matrix: pass.
- Python suite: 133 tests pass.
- Python compile, shell syntax, compiler warnings, Clang static analyzer, and
  `git diff --check`: pass.
- Official MoltenVK 1.4.2/AppKit/Metal probe outside the restricted sandbox:
  pass for magenta clear, black clear, graphics-draw, load-only, compositor
  image, and exact ESO 3420 x 2148 / 3420 x 2146 surface controls.
- Prepared proxy SHA-256:
  `4c1c69fd86bfba13a20f218e072092df8082dc8920e8607895358e85770c49d2`.
- Official MoltenVK SHA-256 remains
  `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.

No Steam, launcher, or ESO process was launched by the agent. The game bundle,
settings, and caches were not modified while preparing this source candidate.

## Procedure if approved

1. Require the shared bundle-idle gate; idle Steam alone is not a blocker.
2. Restore the installed Experiment 0030 proxy to pristine with cache
   preservation, rebuild from this committed source, and install only
   `startup-compositor-neutralize`.
3. Verify exact source/proxy/runtime/marker, settings, and cache identities and
   prepare a fresh evidence boundary.
4. The user performs one ordinary Steam-path startup and reports only whether
   pink appeared. No gameplay, settings change, or screenshot is required.
5. Analyze the exact run with
   `tools/analyze_startup_compositor_window_neutralize.py`, collect evidence,
   and restore `performance-aggressive` unless promotion is justified.

## Pass/fail criteria

Pass requires no visible pink, suppression spanning the proven magenta window,
one ordinal-150 forward latch, bounded ordinal-180 finish, no unexpected
readback/error/fallback, unchanged settings and preserved backups, and no new
crash report. A visually clean run without exact log coverage is inconclusive.
A mechanically complete window with persistent pink is a failed repair and
selects Experiment 0029 or the pre-present exact-pixel alternative.

## Installation state

The user approved installation and also established standing project
authorization for future verified cache-preserving restore/install cycles.
That rule is committed in `AGENTS.md`; interactive Steam/launcher/ESO launches
remain user-owned.

Evidence boundary `artifacts/experiment-0031-20260801T174431Z` was prepared
from repository commit `5b152fe`, containing exact bridge source checkpoint
`1dacfd2`. The bundle-idle gate passed with idle Steam open and no ESO file or
update activity. A cache-preserving pristine restore, clean rebuild, and
`startup-compositor-neutralize` installation then completed.

Installed and built proxy SHA-256 values match exactly at
`4c1c69fd86bfba13a20f218e072092df8082dc8920e8607895358e85770c49d2`.
The official MoltenVK remains `aef00b13...`; the quick gate is `READY` and all
three pipeline-cache identities pass. Installation preserved:

```text
settings:     297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c
active cache: ad3f6ba04c03e685d89c1f3806fb48f17ac9d521ea041331a1e286d4b23711bb
old backup:   72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
```

No Steam, launcher, or ESO process was launched by the agent. One bounded
user-controlled startup is now the active gate; gameplay and settings changes
are not required.
