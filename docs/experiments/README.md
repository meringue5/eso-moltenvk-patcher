# Experiment index

This directory is the durable history of controlled research runs. Experiment
records own run-specific intent, procedure, evidence, results, interpretation,
and rollback state. Current project state belongs in `docs/STATUS.md`.

## Records

| ID | Date | Subject | Outcome | Record |
|---:|---|---|---|---|
| 0001 | 2026-07-19 | MoltenVK 1.4.1 full redirect | Failed after activation; rolled back | [Run](0001-moltenvk-1.4.1-full-redirect.md), [crash analysis](0001-crash-analysis.md) |
| 0002 | 2026-07-19 | Live-check proc-traced startup | Running; awaiting user startup | [Run](0002-live-check-proc-trace-startup.md) |

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
