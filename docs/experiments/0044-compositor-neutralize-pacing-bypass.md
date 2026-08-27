# Experiment 0044: fixed-window compositor neutralization with pacing bypass

- Date: 2026-08-27
- Outcome: **candidate installed; user launch pending**
- Rollback: **not started; Experiment 0044 remains installed and active**

## Question

Can the already validated fixed-window pink neutralizer remove the now-proven
GUI-input magenta while the independent exact host-loop pacing bypass preserves
normal FPS, without enabling pixel readback or changing MoltenVK, caches,
settings, AppKit state, or launcher behavior?

## Evidence selecting this combination

Experiment 0043 exact run `20260827T054945.100479000Z-pid33219` had normal FPS
with visible pink and decisively localized exact magenta to the compositor's
GUI-classified second image. The same descriptor/image identity later contained
ordinary colors, proving an in-place content transition rather than a replaced
image descriptor.

Experiment 0031 already removed visible pink twice by replacing only the exact
final-compositor draws from generation-2 ordinals 71 through 149 with opaque
black and forwarding at ordinal 150. Experiment 0041 separately patches only
ESO's exact inactive 100-ms host-loop sleep. Experiment 0043 retained the more
expensive forward-only image audit and still produced normal FPS, further
weakening tracking overhead as a necessary cause of the low-FPS state.

## Controlled change

Add isolated source-maintenance mode
`startup-compositor-neutralize-pacing-bypass`. It composes existing, separately
tested mechanisms without changing either implementation:

- the Experiment 0031 exact pipeline/layout/generation/ordinal neutralizer;
- the Experiment 0041 exact 12-byte inactive pacing patch;
- non-maximized MoltenVK pipeline compilation and bounded first-64 pipeline
  timing retained from the current diagnostic baseline; and
- default-`info` visibility for the bounded neutralizer begin, maximum 96
  suppression, and single terminal latch records.

The mode does not enable swapchain or compositor-image pixel readback. It does
not sample or rewrite the underlying GUI image, alter descriptors or shaders,
synthesize focus events, force the active byte, change AppKit callbacks, modify
settings or caches, or bypass Steam authentication. Any incomplete exact
compositor state fails open by forwarding ESO's original draw.

## Gates

Before installation require:

1. exact ESO 12.0.8 update/profile match;
2. complete source rebuild with Bink re-export and Rosetta patch probes;
3. lifecycle fixed-window and inactive-pacing probes;
4. combined MoltenVK configuration probe;
5. analyzer coverage for the combined mode and exact pacing activation;
6. bounded log-policy probe at the production `info` level;
7. all Python, release transaction, syntax, and diff checks;
8. official and embedded Metal-backed non-game probes; and
9. shared bundle-idle, pristine restore, payload identity, cache identity, and
   settings-preservation gates.

## Planned user gate

One ordinary Steam-path launch. The user reports both pink visibility and FPS
state. Pass requires:

- no visible pink;
- normal FPS from startup through the normal scene;
- exact combined mode and pacing activation records;
- 79 contiguous exact-target suppressions from ordinals 71 through 149;
- one ordinal-150 `present-deadline` forwarding latch;
- bounded ordinal-180 finish;
- 64 successful non-null retained pipeline calls; and
- no lifecycle error, overflow, pixel-readback activation, or new crash.

A clean visual result without the exact records is inconclusive. Any low-FPS
result fails the candidate regardless of pink suppression.

## Preparation result

The isolated mode, production log classification, configuration probe, and
combined-mode analyzer coverage are implemented. Current local evidence:

```text
fresh bridge build: PASS
Bink re-export and Rosetta self-patch: PASS
inactive pacing and lifecycle neutralizer probes: PASS
bounded log-policy probe: PASS
combined MoltenVK configuration: PASS
Python tests: 136 PASS
release installer transaction regression: PASS
Python compile, shell syntax, git diff check: PASS
official MoltenVK 1.4.2 Metal compatibility/surface probes: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probes: PASS on Apple M4
bridge SHA-256: 7b5aa1b0b58729b5faf7adbbc503460c3e07cc1dba54159107a37ec6b480787f
retagged original Bink SHA-256: f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

No running game or bundle file was modified during source preparation.

After ESO and the launcher closed, the shared bundle-idle gate allowed the
transaction with idle Steam because it had no ESO file or update activity. The
installed 0043 bridge was restored to the verified pristine loader while
preserving the active cache, old-runtime cache backup, and `UserSettings.txt`:

```text
active pipeline cache SHA-256: a3cc45c753c8158ba81dfd14caf8acff34ef44f64f6f471cf066a9b06be6413d
old-runtime cache backup SHA-256: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
UserSettings.txt SHA-256: 104e894803e70dae30fdab887474a8f3116387375614484d36c3755c58745fb0
```

The same idle gate passed for installation. Post-install status recognizes the
exact ESO 12.0.8 target and all three pipeline-cache identities. The marker is
exactly `startup-compositor-neutralize-pacing-bypass`; the bridge re-exports
the retagged original Bink; all installed payload hashes match the prepared
build; and all three user-file hashes remain unchanged. No game or launcher was
started by the agent. One ordinary user-controlled Steam-path launch remains.

## Rollback

Not started. The verified pristine loader remains available and was exercised
during the cache-preserving restore/install transaction.
