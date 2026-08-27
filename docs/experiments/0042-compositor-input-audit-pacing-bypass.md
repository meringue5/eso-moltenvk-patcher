# Experiment 0042: compositor input audit with inactive pacing bypass

- Date: 2026-08-27
- Outcome: **source and non-game gates passed; installation pending**
- Rollback: **not started; Experiment 0041 remains installed**

## Question

During ESO's visible canonical-magenta startup frame, does the final compositor
receive magenta directly from retained MSL `Sampler0` (scene) or `Sampler1`
(GUI), and does that input change descriptor identity or contents when the
normal scene first appears?

## Controlled change

This experiment reuses the completed but never game-run two-input sampler from
Experiment 0029. A new isolated mode,
`startup-compositor-audit-pacing-bypass`, keeps Experiment 0041's exact
inactive 100 ms sleep bypass, non-maximized MoltenVK compilation policy,
bounded pipeline timing, settings, and caches. It adds only the bounded
forward-only startup pixel, draw, descriptor-class, and two-input image audit
through generation 2 ordinal 180.

The audit reads five points from each bound compositor input after queue-idle
synchronization. It does not replace a draw, clear an attachment, alter a
descriptor, synthesize focus, or suppress the visible pink frame. The existing
analyzer must classify one of scene-input magenta, GUI-input magenta, multiple
magenta inputs, combined-input candidate, or inconclusive.

## Safety and evidence gates

- Exact target remains ESO 12.0.8, databuild `3288357`, SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- `scripts/check-update.sh` reports `CURRENT` before candidate preparation.
- Experiment 0041 remains installed while the user controls ESO; no bundle
  mutation occurs during source preparation.
- Before installation, require a verified restore path, rebuilt source,
  synthetic lifecycle/image probes, configuration probe, Python tests, shell
  syntax, `git diff --check`, and both Metal-backed non-game Vulkan probes.
- Installation may begin only after ESO and the launcher are closed and the
  shared bundle-idle gate passes. Preserve both pipeline caches and settings.

## Planned user gate

One ordinary user-controlled Steam-path launch is sufficient if visible pink
occurs and the bounded audit reaches ordinal 180 without an audit error. The
user reports only whether pink appeared and whether FPS was normal. The agent
will classify the captured run before designing any pixel-changing repair.

## Result

The combined source candidate is complete. No game, launcher, bundle file,
setting, or cache was changed during preparation.

```text
selected target/update gate: CURRENT, ESO 12.0.8 databuild 3288357
fresh complete bridge build: PASS
Bink re-export and Rosetta self-patch: PASS
inactive pacing exact-target probe: PASS
lifecycle compositor image identity/subresource probe: PASS
new MoltenVK mode configuration: PASS, non-maximized compilation
Python tests: 135 PASS
release installer transaction regression: PASS
Python compile, shell syntax, git diff check: PASS
official MoltenVK 1.4.2 Metal compatibility/surface probes: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probes: PASS on Apple M4
bridge SHA-256: 475ce59dc8af503f3231900fc090b5173eddff1f8107c364a8a927b42c976668
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
original Bink SHA-256: f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
```

Clang's static analyzer completed and reproduced two existing stream-state
warnings in the executable SHA-256 read loop at `mvk_shim.c:323`; neither is
in or reached from this mode-selection change. The warnings are preserved as
known analyzer debt rather than being silently described as a clean pass.

Installation and the user-controlled launch remain pending.

## Interpretation

Confirmed before this experiment: the final swapchain contains exact magenta,
and the same final compositor pipeline draws both the magenta interval and the
later normal scene. The two-input implementation and its non-game controls
already existed in Experiment 0029, but it was not installed or run against
ESO. This experiment changes the execution profile so that diagnostic evidence
does not discard the current FPS-priority pacing bypass.

## Rollback

Not started. Experiment 0041 remains installed with its pristine restore path
and caches verified.
