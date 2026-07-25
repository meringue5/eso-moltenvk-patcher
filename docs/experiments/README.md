# Experiment index

This directory is the durable history of controlled research runs. Experiment
records own run-specific intent, procedure, evidence, results, interpretation,
and rollback state. Current project state belongs in `docs/STATUS.md`.

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
| 0011 | 2026-07-25 | MTLHeap disabled during live reset | Planned | [Run](0011-mtlheap-disabled-live-reset.md) |

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
