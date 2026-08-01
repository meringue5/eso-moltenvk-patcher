# Experiment 0030: startup compositor placeholder neutralization

- Date: 2026-08-02
- Outcome: **failed**
- Rollback: **not started; candidate installed**

## Question

Can the visible canonical-magenta startup interval be removed without changing
MoltenVK configuration, caches, settings, shaders, or steady-state gameplay by
withholding only the already isolated final-compositor placeholder draw?

## Evidence basis

Experiments 0027 and 0028 establish the intervention boundary without another
diagnostic run:

- generation-2 presents through ordinal 70 are black and contain no draw;
- ordinals 80 through 140 contain one indexed fullscreen compositor draw and
  exact `(255,0,255,255)` pixels;
- ordinals 150 onward use the same draw and pipeline but contain the normal
  scene;
- the pipeline, layout, set handles, and push state remain fixed, while the
  aggregate latest-descriptor-update signature is stable throughout the
  sampled magenta interval and changes at the scene interval.

The preserved MoltenVK cache identifies the draw as ESO's scene-and-GUI final
compositor rather than a failed shader. The visible artifact is therefore
treated as an application placeholder presentation defect, not a gameplay or
driver failure.

## Hypothesis

The first exact target compositor descriptor state is the startup placeholder.
Replacing that target draw with an opaque-black full-surface clear while the
descriptor state remains identical will keep the already black startup surface
black. Forwarding the very first draw whose descriptor signature changes will
show the normal scene without delaying it.

If any exact identity, descriptor completeness, framebuffer extent, or clear
entry point is unavailable, the bridge must forward the original draw. If the
descriptor never changes, a hard present/draw bound must restore forwarding.

## Target and intervention

- ESO 12.0.7, databuild `3281538`.
- Official MoltenVK 1.4.2 SHA-256 `aef00b13...`.
- Exact `performance-aggressive` MoltenVK configuration under isolated marker
  `startup-compositor-neutralize`.
- Indexed pipeline signature `c43e4410d3b33fe7`.
- Pipeline-layout signature `d175d2c1daed112d`.
- Ordered set layouts `e3c2499a89df1706` (three buffers) and
  `d0edad262f8c4230` (two images and three buffers), with no push constants.

The bridge watches only generation 2, beginning at the next present ordinal
60. On the first complete exact-target draw it records the aggregate current
descriptor-update signature and replaces the draw with one opaque-black clear
over the tracked framebuffer extent. Identical target states are treated the
same way. A changed descriptor signature permanently latches to direct
forwarding and forwards that first changed draw itself.

The bridge never suppresses ordinal 150 or later and never suppresses more
than 96 draws. Unexpected non-indexed use, incomplete descriptor/class state,
missing framebuffer/clear state, or any exact-identity mismatch forwards the
application command. Pixel readback is disabled. Normal performance modes do
not enable the bounded tracking tables or intervention.

## Preflight

- `scripts/check-update.sh`: `CURRENT`, exact recognized target.
- `scripts/status.sh`: current official runtime and installed bridge; production
  marker remains present.
- No Steam, launcher, or ESO process was launched by candidate preparation.
- No game-bundle file, setting, cache, or configuration was modified.
- Installation uses the shared bundle-idle gate. Idle Steam is allowed; ESO,
  the launcher, open bundle files, and active Steam content operations are not.
- Explicit approval remains required before modifying the game bundle.

The user explicitly approved `startup-compositor-neutralize` installation.
Evidence boundary `artifacts/experiment-0030-20260801T172534Z` was prepared
from source commit `c04de86`. The locally parsed ESO target, databuild, and all
eight launcher repository IDs remained exact. The latest `noUpdateRequired`
launcher snapshot was 4,874 seconds old, so evidence preparation used the
script's scoped 7,200-second maximum instead of requiring another launcher
interaction; the build/content comparison itself was not weakened.

## Non-game evidence

- Lifecycle wrapper control: stable descriptor state suppresses the indexed
  draw and emits a full-frame black clear: pass.
- Descriptor transition control: the first changed-state draw is forwarded and
  the state permanently latches to forwarding: pass.
- Fail-open control: incomplete exact-target input forwards unchanged and
  records an abort latch: pass.
- Effective MoltenVK configuration matrix, including the new mode: pass.
- Complete bridge build with warnings as errors: pass.
- Python analyzers and controls: 129 tests pass.
- Official MoltenVK 1.4.2/AppKit/Metal probe outside the restricted sandbox:
  BGRA8 magenta, black, load-only, and graphics-draw controls pass at 64-pixel
  and exact 3420 x 2148 / 3420 x 2146 extents; RGBA16F mip/layer control passes.
- Prepared proxy SHA-256:
  `b929afbd24999a6ad08dc542d6683f71d1afad8162c19ad9e1e2a31c9d21c99a`.

The dedicated analyzer requires contiguous exact-target suppression records,
one descriptor-transition forwarding latch, the ordinal-180 bounded finish,
no readback activation, and no lifecycle error, truncation, abort, or deadline
fallback. It classifies the user's independent visual observation separately.

## Procedure

1. Record the current target, runtime, proxy, marker, settings, and cache
   identities without changing them.
2. After explicit approval and a passing bundle-idle gate, restore the pristine
   loader with cache preservation, rebuild, and install only
   `startup-compositor-neutralize`.
3. The user launches through the ordinary Steam-authenticated path. The agent
   does not launch Steam, the launcher, or ESO.
4. The user reports whether the full-screen pink interval appeared. No gameplay
   duration, setting change, screenshot, or extra interaction is required.
5. Analyze the exact run with
   `tools/analyze_startup_compositor_neutralize.py` and require the fail-open
   and transition invariants above.
6. If the visual and log gates pass, evaluate promotion into the packaged
   production mode. Otherwise restore `performance-aggressive` with settings
   and caches preserved.

## Result

The source candidate and all agent-only gates are complete. The shared
bundle-idle gate passed while Steam remained open with no ESO bundle file or
content-update activity. A cache-preserving pristine restore, clean rebuild,
and approved `startup-compositor-neutralize` installation completed.

The installed proxy matches prepared SHA-256
`b929afbd24999a6ad08dc542d6683f71d1afad8162c19ad9e1e2a31c9d21c99a`
byte for byte. The installed official MoltenVK remains
`aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
The marker is exactly `startup-compositor-neutralize`. Installation preserved:

```text
settings:     297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c
active cache: 234dc3189fcd2156e9de984a8aec5d5b87a66e8a6e39f7b4f081df851019a7b8
old backup:   72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
```

No Steam, launcher, or ESO process was launched by the agent.

The user then performed the single bounded startup and reported that the pink
interval remained visible. The exact eligible run is
`20260801T172806.047658000Z-pid17050`. It contains one suppression at
generation 2 ordinal 71 with descriptor signature `593490cb014fd20f`, followed
immediately by a permanent descriptor-transition forwarding latch at ordinal
72 with signature `f9e28936d0b40ea9`. The known visible magenta samples from
the prior exact audit begin at ordinal 80 and continue through ordinal 140.
The candidate therefore forwarded the entire proven magenta interval; it did
not test whether replacing those frames' compositor draw would remove pink.

The bounded audit finished at ordinal 180 without lifecycle error, detail
truncation, or pixel-readback activation. No new crash report appeared.
`UserSettings.txt` retained exact normalized and structural identity with SHA-256
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.
The active 1.4.2 pipeline cache underwent a normal runtime update to SHA-256
`ad3f6ba04c03e685d89c1f3806fb48f17ac9d521ea041331a1e286d4b23711bb`;
the preserved old backup remained byte-identical at
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

## Interpretation

Confirmed: the implementation matched the exact compositor identity and
replaced one early draw, but its draw-recording-time descriptor signature
changed at ordinal 72. That is eight presents before the first proven magenta
sample and 78 presents before the first proven scene sample. The intervention
therefore disarmed before the interval it was meant to cover.

The original analyzer's `COMPOSITOR-PLACEHOLDER-EXCLUDED` classification was
invalid for this trace because it checked that a transition occurred but did
not require suppression to span the known magenta interval. The analyzer now
requires coverage through ordinal 140 and a forwarding latch no earlier than
ordinal 150; this exact run correctly returns `INCONCLUSIVE` with an early-latch
reason.

Inference: Experiment 0028's sampled-present aggregate transition cannot be
used as “the first changed draw-recording signature.” Transitional compositor
states exist at ordinals 71 and 72 before the stable sampled-magenta interval.
This result does not exclude the compositor-placeholder cause and does not
justify changing assets, shaders, settings, or caches.

## Rollback

Not started. The candidate is installed. The pristine loader and
cache-preserving restore path were checked immediately before installation;
the production `performance-aggressive` marker can be restored without
changing either retained cache or settings state.

## Follow-up

Restore `performance-aggressive` when the bundle-idle gate permits it. Preserve
this early-latch result. A successor must either arm only at the independently
proven ordinal-80 boundary and require coverage through ordinal 140, or detect
and replace exact final-image magenta immediately before presentation. It must
pass static, synthetic, and real non-game synchronization controls before any
new installation or user startup. Experiment 0029 remains available if the
two compositor inputs must be distinguished.
