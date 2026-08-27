# Experiment 0042: compositor input audit with inactive pacing bypass

- Date: 2026-08-27
- Outcome: **inconclusive; default production log filtered audit evidence**
- Rollback: **available and verified; caches and settings preserved**

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
- Experiment 0041 remained installed while the user controlled ESO; no bundle
  mutation occurred during source preparation.
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

## Installation state

The shared bundle-idle gate found ESO and the ZeniMax launcher closed. Steam
remained open but held no ESO file and had no ESO update activity. The
cache-preserving restore produced the exact original loader state with the
enable marker absent. Both pipeline caches and `UserSettings.txt` retained
their pre-restore hashes.

The same gate passed again immediately before installing only
`startup-compositor-audit-pacing-bypass`. Post-install update and status checks
select the same exact 12.0.8 target, report the bridge and official MoltenVK
current, and pass all three pipeline-cache identity checks. Installed payloads
match the built candidate:

```text
marker:          startup-compositor-audit-pacing-bypass
bridge:          475ce59dc8af503f3231900fc090b5173eddff1f8107c364a8a927b42c976668
original Bink:   f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK:        aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
active cache:    34437fdc95001f02ee3e9bdf8c896236af85bc96f0078c8a6bdfcf3256d65fc3
old cache:       72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
UserSettings:    104e894803e70dae30fdab887474a8f3116387375614484d36c3755c58745fb0
```

No game, launcher, or Steam process was launched by the agent. The one
user-controlled launch remained pending at installation time.

## User result and audit failure

The user completed one ordinary Steam-path launch. Exact run
`20260827T053611.563842000Z-pid26073` selected the intended combined mode,
loaded official MoltenVK 1.4.2, installed the inactive pacing bypass, and
redirected all 17 entry points. The user reported visible pink and normal FPS.
The internal active state was `active=yes`; graphics-pipeline call 5 began at
12.330 seconds and returned successfully in 1.455 milliseconds. All 64 bounded
pipeline calls returned `VK_SUCCESS` with non-null output.

The dedicated compositor analyzer returned `INCONCLUSIVE`. The log contains
the mode record and pipeline timing but none of the color, present-pixel, draw,
input, descriptor-class, or compositor-image records. Source inspection
identifies the exact integration failure: the production log classifier emits
only `info` by default, while these bounded audit records were classified as
`debug` or `trace`. The audit ran in memory, but its evidence was discarded
before writing. This run cannot distinguish scene from GUI and must not be
reinterpreted as absence of magenta input.

The normal-FPS result is still a useful no-regression observation for the
forward-only tracking path. It does not validate the audit's evidence path.
Experiment 0043 repairs only bounded audit log visibility before requesting a
replacement launch.

## Interpretation

Confirmed before this experiment: the final swapchain contains exact magenta,
and the same final compositor pipeline draws both the magenta interval and the
later normal scene. The two-input implementation and its non-game controls
already existed in Experiment 0029, but it was not installed or run against
ESO. This experiment changes the execution profile so that diagnostic evidence
does not discard the current FPS-priority pacing bypass.

## Rollback

The pristine loader remains the verified restore source. The cache-preserving
restore/install cycle proved the rollback path before installing 0042, and no
cache or setting was replaced.
