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

## Maintenance rules

- Record the search date, scope, and important negative searches.
- Link primary sources directly and distinguish their observations from this
  project's inferences.
- Add a dated amendment when later evidence changes a conclusion.
- Keep local launches, crash evidence, and rollback state in `../experiments/`.
