# Experiment 0033: production-refactor release validation

- Date: 2026-08-02
- Outcome: **succeeded; approved as replacement 0.1.0 payload**
- Rollback: **not required; validated build remains installed**

## Question

Does the cleaned MoltenVK 1.4.2 production build remain correct in ordinary
gameplay after abandoned 1.4.1 paths are removed, and is High subsampling safe
for the bundled M4 settings template?

## Target and change set

- Exact ESO macOS 12.0.7 target, databuild `3281538`.
- Official MoltenVK 1.4.2.
- Removed dead 1.4.1 backport, comparison probes, and failed legacy feature
  masking; the 1.4.2 production redirect and compositor repair are unchanged.
- Installed package state: `0.1.1-dev`.
- Built and installed proxy SHA-256:
  `5d6aa40ddd1ac7d7c81a8d164bb0b317a17154034596d63a73bd4710a5139284`.
- Live settings include `SUB_SAMPLING "2"` (High).

## Procedure

The cleaned source passed its complete non-game build and release transaction.
The exact resulting proxy was installed through the project package. The user
launched ESO through the normal Steam authentication path and performed
ordinary gameplay with High subsampling. The agent did not launch ESO, Steam,
or the launcher.

## Evidence

The installed proxy and freshly built proxy hashes match exactly. Installation
state records `0.1.1-dev`; the active runtime is the pinned official MoltenVK
1.4.2 and the exact target remains current.

Run `20260802T094941.290510000Z-pid95867` records:

```text
redirected entry points: 17
startup strategy:        ordinal-window, fail-open
suppressed draws:        79
forward latch:           generation 2, ordinal 150
```

The user reports that actual gameplay completed without a problem. The live
settings file records High subsampling. No fixed-duration performance
measurement was requested for this release replacement smoke test.

## Result

The exact cleaned production binary started and played normally, and High
subsampling produced no reported problem on the validated M4 baseline.

## Interpretation

Confirmed: removing the abandoned 1.4.1 code paths did not regress the current
1.4.2 production behavior in this supported configuration. High subsampling is
eligible for the bundled standard template.

This validation supports replacing the original 0.1.0 payload without changing
its supported ESO, hardware, launcher, or fail-closed boundaries.

## Follow-up

Build version `0.1.0` from this exact source and settings template, repeat all
non-game and release transaction gates, update the existing release asset and
checksum, and move `v0.1.0` to the replacement release commit.
