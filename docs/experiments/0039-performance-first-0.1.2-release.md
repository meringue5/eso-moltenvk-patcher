# Experiment 0039: performance-first 0.1.2 release

- Date: 2026-08-25
- Outcome: **running; release candidate passed all local gates**
- Rollback: **not applicable to package assembly; installed 0038 control retained**

## Question

Can the exact Experiment 0038 performance-first control be promoted to a
public maintenance release without changing its runtime, cache, settings, ESO
target, update-recovery boundary, or reversible installer transaction?

## Product decision

Low-FPS avoidance is the release blocker. Pink startup suppression is optional.
The user accepted the Experiment 0038 result—normal FPS with the original pink
placeholder visible—and explicitly selected it for release.

Version 0.1.2 therefore:

- retains official MoltenVK 1.4.2 and ESO 12.0.8/databuild `3288357` support;
- retains the 0.1.1 exact-target and compatible-update auditor;
- selects `startup-pipeline-timing-control` in every installer surface;
- keeps non-maximized pipeline compilation and the bounded first-64-call timing;
- removes the compositor draw substitution and its supporting startup audits;
- preserves caches and settings under the existing transaction policy; and
- documents visible pink as a known cosmetic issue, not patch failure.

This release does not claim that one normal control proves long-term
reliability or the exact internal race. It packages the user's explicit
performance-first tradeoff and retains the full negative-result history.

## Gates

1. Rebuild from committed source and pass all non-game probes.
2. Pass Python tests, compile checks, shell syntax, and `git diff --check`.
3. Pass the disposable release transaction with an exact assertion that the
   installed marker is `startup-pipeline-timing-control`.
4. Assemble `ESO-MoltenVK-Patcher-0.1.2.zip`, verify every internal checksum,
   archive hygiene, and the payload/runtime hashes.
5. Merge to current remote `main`, tag `v0.1.2`, publish the ZIP and SHA-256,
   then verify the public release metadata and downloaded asset digest.

## Result

All local gates passed:

- complete bridge build with warnings as errors;
- Bink re-export, Rosetta self-patch, HDR compatibility, lifecycle, reset, and
  render-audit probes;
- exact MoltenVK configuration probes for `performance-aggressive`, the former
  neutralizer, and `startup-pipeline-timing-control`; the release mode verified
  `maximize_concurrent_compilation=0`;
- 134 Python tests, Python compile, shell syntax, and `git diff --check`;
- disposable installer interruption, recovery, install, remove, reinstall,
  settings, and both compatible-update loader-state transactions; and
- complete ZIP checksum and archive-hygiene verification.

Prepared artifact:

```text
ZIP:             ESO-MoltenVK-Patcher-0.1.2.zip
ZIP SHA-256:     7b587caa68bf729ec4ea75888223c72908672fb70c853a382d7215407d21e830
bridge SHA-256:  837ce644b3fa133ae8a5322fff8c0947d6fb8ccfc8c8d205f20c230b746d1580
MoltenVK SHA-256:aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
marker:          startup-pipeline-timing-control
```

The packaged bridge is byte-identical to the currently installed and
user-validated Experiment 0038 bridge. Publication, public asset digest
verification, and branch cleanup remain pending.
