# Experiment index

This directory is the durable history of controlled research runs. Experiment
records own run-specific intent, procedure, evidence, results, interpretation,
and rollback state. `teso4m4` was promoted to production on 2026-08-01; these
records remain historical evidence and do not describe the current product
classification. Current project state belongs in `docs/STATUS.md` and the
production scope belongs in `docs/PRODUCTION.md`.

## Records

| ID | Date | Subject | Outcome | Record |
|---:|---|---|---|---|
| 0001 | 2026-07-19 | MoltenVK 1.4.1 full redirect | Failed after activation; rolled back | [Run](0001-moltenvk-1.4.1-full-redirect.md), [crash analysis](0001-crash-analysis.md) |
| 0002 | 2026-07-19 | Live-check proc-traced startup | Failed after activation; rolled back | [Run](0002-live-check-proc-trace-startup.md) |
| 0003 | 2026-07-19 | Original-runtime long session | Inconclusive as override test; baseline captured | [Run](0003-original-runtime-long-session.md) |
| 0004 | 2026-07-19 | HDR-advertisement-filtered startup | Failed at confirmed NULL HDR setter; evidence preserved, later restored for rebuild | [Run](0004-hdr-advertisement-filter-startup.md) |
| 0005 | 2026-07-19 | Exact HDR surface-format filter | Succeeded at startup; failed rendering correctness | [Run](0005-hdr-surface-format-filter-startup.md) |
| 0006 | 2026-07-19 | Metal argument buffers disabled | Baseline lobby/world rendering passed; live SSAO toggle failed | [Run](0006-metal-argument-buffers-disabled.md) |
| 0007 | 2026-07-20 | ESO 12.0.7 target rebase | Succeeded at rebase and short world rendering; extended stability incomplete | [Run](0007-eso-12.0.7-target-rebase.md) |
| 0008 | 2026-07-21 | Skip pregame videos | Failed hypothesis; videos skipped but hot-pink frame persisted | [Run](0008-skip-pregame-videos.md) |
| 0009 | 2026-07-25 | Live resolution reset | Failed rendering correctness; solid-color output after reset | [Run](0009-live-resolution-reset.md) |
| 0010 | 2026-07-25 | Swapchain lifecycle trace | No swapchain fault found; reset corruption persisted | [Run](0010-swapchain-lifecycle-trace.md) |
| 0011 | 2026-07-25 | MTLHeap disabled during live reset | Failed; solid-color reset corruption persisted | [Run](0011-mtlheap-disabled-live-reset.md) |
| 0012 | 2026-07-25 | Bounded reset-resource trace | Failed rendering; localized to descriptor/resource state | [Run](0012-reset-resource-trace.md) |
| 0013 | 2026-07-25 | Command pooling disabled during live reset | Failed; solid-color corruption persisted | [Run](0013-command-pooling-disabled.md) |
| 0014 | 2026-07-25 | Bounded render-graph audit | Diagnostic passed; rendering failed and cache path became leading target | [Run](0014-render-graph-audit.md) |
| 0015 | 2026-07-25 | Reset-only pipeline-cache bypass | Failed; complete cache bypass did not correct rendering | [Run](0015-reset-pipeline-cache-bypass.md) |
| 0016 | 2026-07-25 | Full-lifetime reset-state audit | Inconclusive; three reset windows overflowed and the audit degraded performance | [Run](0016-full-lifetime-reset-audit.md) |
| 0017 | 2026-07-25 | Swapchain Metal texture-cache fix | Failed ESO applicability; FPS restored, resolution reset blacked out | [Run](0017-swapchain-texture-cache-fix.md) |
| 0018 | 2026-07-26 | Embedded-runtime core feature profile | Failed; exact mask still produced solid output | [Run](0018-legacy-feature-profile.md) |
| 0019 | 2026-07-26 | Combined performance-safe execution path | Failed reset repair; low-overhead path active without obvious other issue | [Run](0019-performance-safe-path.md) |
| 0020 | 2026-07-26 | Live-resource descriptor performance | Succeeded in ordinary use; non-game descriptor encoding improved 10.1% | [Run](0020-live-resource-performance.md) |
| 0021 | 2026-07-26 / 2026-08-01 | Official MoltenVK 1.4.2 maintenance adoption | Succeeded in ordinary high-settings gameplay; live graphics resets render correctly | [Run](0021-moltenvk-1.4.2-maintenance.md) |
| 0022 | 2026-08-01 | Transient startup surface analysis | Screenshot amendment proves exact sRGB magenta persists beyond the first-surface boundary | [Run](0022-startup-surface-analysis.md) |
| 0023 | 2026-08-01 | Bounded startup color audit candidate | Non-game mechanism succeeded; generation-1-only scope invalidated before installation | [Run](0023-startup-color-audit-candidate.md) |
| 0024 | 2026-08-01 | Two-generation startup color audit redesign | Succeeded; exact pink run contained only opaque-black clears, normal profile restored | [Run](0024-two-generation-startup-color-audit.md) |
| 0025 | 2026-08-01 | Bounded startup FX-sentinel neutralization | Inconclusive; exact hook installed but initializer had zero calls, normal profile restored | [Run](0025-startup-fx-sentinel-neutralization.md) |
| 0026 | 2026-08-01 | Bounded startup pre-present pixel audit | Succeeded; exact magenta confirmed in final swapchain content before present, normal profile restored | [Run](0026-startup-present-pixel-audit.md) |
| 0027 | 2026-08-01 | Bounded startup presented-draw audit | Succeeded; isolated the sole indexed draw/pipeline writing both magenta and later scene frames, normal profile restored | [Run](0027-startup-draw-audit.md) |
| 0028 | 2026-08-01 / 2026-08-02 | Bounded startup draw-input provenance audit | Succeeded; stable sets/layout/push but descriptor update changed from magenta to scene, normal profile restored with caches and settings preserved | [Run](0028-startup-input-provenance.md) |
| 0029 | 2026-08-02 | Bounded startup scene/GUI compositor input audit | Planned; source and non-game gates pass, production profile unchanged | [Run](0029-startup-compositor-input-audit.md) |
| 0030 | 2026-08-02 | Bounded startup compositor placeholder neutralization | Failed coverage; latched at ordinal 72 before the proven magenta interval, rollback complete | [Run](0030-startup-compositor-placeholder-neutralization.md) |
| 0031 | 2026-08-02 | Fixed-window startup compositor neutralization | Succeeded twice; 79 exact draws neutralized through ordinal 149 and scene forwarded at 150 | [Run](0031-startup-compositor-window-neutralization.md) |
| 0032 | 2026-08-02 | 0.1.0 release-candidate end-to-end validation | Succeeded; public package installed, started, and played normally | [Run](0032-release-candidate-end-to-end.md) |
| 0033 | 2026-08-02 | Production-refactor release validation | Succeeded; exact cleaned build and High subsampling played normally | [Run](0033-production-refactor-release-validation.md) |
| 0034 | 2026-08-11 | ESO 12.0.8 update-compatible recovery | Update recovery and gameplay succeeded; cold-start reliability follow-up required | [Run](0034-eso-12.0.8-update-compatible-recovery.md) |
| 0035 | 2026-08-16 | Cold-start compiler-service comparison | Bad and smooth starts both activated the bridge; compiler-service engagement distinguished the smooth restart | [Run](0035-cold-start-compiler-service-comparison.md) |
| 0036 | 2026-08-17 | Runtime compiler-readiness gate | Failed as a cold-start fix; low FPS recurred after the canary's compiler jobs succeeded | [Run](0036-runtime-compiler-readiness-gate.md) |
| 0037 | 2026-08-17 / 2026-08-25 | Serialized startup pipeline compilation | Failed as a reliability fix; recurrence showed ESO delayed issuing otherwise-fast successful pipeline calls | [Run](0037-serialized-startup-pipeline-compilation.md) |
| 0038 | 2026-08-25 | No-neutralizer startup control | Running; timing-only control installed with caches and settings preserved | [Run](0038-no-neutralizer-startup-control.md) |

## Recording policy

- Assign the next zero-padded ID before changing the game bundle.
- Copy [the template](TEMPLATE.md) and give the record a descriptive name in the
  form `NNNN-short-subject.md`.
- Record one controlled configuration per ID. A materially different patch set,
  runtime mode, or launch procedure gets a new ID.
- Use the outcome vocabulary `planned`, `running`, `succeeded`, `failed`,
  `inconclusive`, or `aborted`, followed separately by rollback state.
- Preserve negative results. Add dated amendments instead of rewriting what was
  observed at the time.
- Do not commit raw crash reports, proprietary binaries, account data,
  machine-specific paths, logs, or caches. Store only sanitized evidence and
  hashes needed for reproducibility.
