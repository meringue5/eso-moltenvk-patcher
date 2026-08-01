# Experiment 0029: startup compositor input audit

- Date: 2026-08-02
- Outcome: **planned**
- Rollback: **not started; production profile remains installed**

## Question

Does canonical magenta come from the final compositor's `Sampler0` scene image
or `Sampler1` GUI image, and is the responsible descriptor replaced or does a
stable image receive new contents?

## Hypothesis

One of the two images will contain near/exact linear magenta throughout the
scheduled magenta presentations and stop doing so in later scene samples. A
changed binding signature identifies descriptor/placeholder replacement; a
stable signature identifies an in-place content transition.

If neither input directly contains magenta, the two-input identity is
incomplete, a subresource is unsupported, or present provenance is missing,
the result is inconclusive. None of those cases permits suppressing the draw.

## Target and change set

- ESO 12.0.7, databuild `3281538`.
- Official MoltenVK 1.4.2 SHA-256 `aef00b13...`.
- Exact `performance-aggressive` configuration under isolated marker
  `startup-compositor-audit`.
- Presented pipeline `c43e4410d3b33fe7`; vertex/fragment hashes
  `c8307556011c995e` / `6907bd3576e3a930`.
- Set layouts `e3c2499a89df1706` (three buffers) and `d0edad262f8c4230`
  (two images and three buffers).

The bridge retains each image descriptor's binding, array element, image view,
sampler, layout, base mip, base array layer, and update signature. At the same
twenty pre-present samples, it reads five points from each compositor image
after the proven same-queue semaphore edge. Binding order maps to retained MSL
`Sampler0` (scene) and `Sampler1` (GUI). Descriptor copy, missing state,
unsupported view/format, overflow, or skipped readback fails closed.

Normal performance modes do not enable these tables or readbacks.

## Preflight

- The 1.4.2 update and selected-target checks are current.
- The installed marker remains `performance-aggressive`; settings and caches
  were not changed during candidate preparation.
- `scripts/build.sh` can use the verified pristine loader as immutable link
  input while a bridge is installed; building no longer requires a temporary
  bundle restore.
- Installation still requires the shared bundle-idle gate: ESO and launcher
  absent, no open bundle file, and no active Steam content operation. Idle
  Steam is not a blocker.
- Explicit approval remains required before modifying the game bundle.

## Procedure

1. Record target, installed profile, runtime, proxy, settings, and cache
   identities without changing them.
2. After explicit approval, restore the pristine loader with cache preservation,
   rebuild, and install only `startup-compositor-audit`.
3. The user launches through the ordinary Steam-authenticated path. The agent
   does not launch Steam, the launcher, or ESO.
4. The user reports only whether pink appeared. No gameplay or setting change
   is requested; capture ends at generation-2 present ordinal 180.
5. Require twenty aligned final-pixel, draw, pipeline, input, descriptor-class,
   and two-image samples with no skip, error, or overflow.
6. Restore `performance-aggressive` with settings and caches preserved.

## Evidence

Candidate gates completed before installation:

- complete bridge build and all effective-configuration probes: pass;
- lifecycle image identity/subresource forwarding probe: pass;
- analyzer controls for scene descriptor replacement, GUI in-place content
  change, combined-input fallback, and missing evidence: pass;
- 125 Python tests: pass;
- warnings-as-errors and Clang static analyzer: pass;
- real official MoltenVK/AppKit BGRA8 magenta, black, load, and draw controls at
  small and exact ESO extents: pass;
- real RGBA16F array-image control at mip 1, layer 2, effective 16 x 16 extent:
  five exact `(1,0,1,1)` samples, pass;
- prepared proxy SHA-256
  `69851209652991de8fd1e0ac87173f01e80e532587ead4d4cb94114b9bc709fa`.

No Steam, launcher, or ESO process was launched by candidate preparation. No
game-bundle file, setting, or cache was modified.

## Result

The source candidate and non-game gate are complete. Installation and the one
bounded user-controlled startup have not begun.

## Interpretation

Confirmed: the audit preserves two image identities through the exact
draw-submit-semaphore-present chain and reads the selected mip and array layer
in both BGRA8 and RGBA16F real MoltenVK controls.

Inference: one valid run should select scene input, GUI input, or a combined
explanation and distinguish descriptor replacement from in-place content
change. It does not yet identify ESO's result.

Hypothesis: a canonical-magenta placeholder occupies one compositor image. No
neutralization is enabled by this experiment.

## Rollback

Not started. The installed `performance-aggressive` profile remains the
production baseline; the prepared build has not entered the game bundle.

## Follow-up

Implement the smallest startup-only neutralization selected by the result. It
must latch permanently to direct forwarding at the first valid input and must
not retain readback in production mode.
