# Research index

This directory contains dated reviews of external work that informs `teso4m4`.
These notes synthesize repositories, issues, and upstream documentation; they
do not replace local experiment records or establish the current safety state.

## Reviews

- [GitHub precedents for replacing a bundled MoltenVK runtime](github-moltenvk-runtime-replacement-precedents.md)
  (2026-07-19): closest public game/runtime replacement attempts, observed
  compatibility failures, and implications for ESO's statically linked runtime.
- [MoltenVK 1.0.18-to-1.4.1 rendering compatibility delta](moltenvk-rendering-compatibility-delta.md)
  (2026-07-19): upstream configuration and descriptor-path differences relevant
  to Experiment 0005's hot-pink frame and black-layer flicker.
- [MoltenVK SSAO and stuttering post-mortem context](moltenvk-ssao-stutter-postmortem.md)
  (2026-07-19): pipeline-cache guidance, adjacent depth/render-pass precedent,
  and the limits of interpreting Experiment 0006's SSAO transition.
- [MoltenVK descriptor behavior at the live-reset boundary](moltenvk-descriptor-reset-delta.md)
  (2026-07-25): official 1.0.18/1.4.1 descriptor implementation comparison
  joined to Experiment 0012's reset-resource trace.
- [MoltenVK 1.4.2 adoption review](moltenvk-1.4.2-adoption-review.md)
  (2026-07-26): official release identity, ESO fix applicability, exact
  Experiment 0020 non-game validation, performance comparison, and
  pipeline-cache transition requirements.

## Maintenance rules

- Record the search date, scope, and important negative searches.
- Link primary sources directly and distinguish their observations from this
  project's inferences.
- Add a dated amendment when later evidence changes a conclusion.
- Keep local launches, crash evidence, and rollback state in `../experiments/`.
