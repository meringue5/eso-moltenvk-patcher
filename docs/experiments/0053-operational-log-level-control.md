# Experiment 0053: operational log-level control

- Date: 2026-08-30
- Outcome: **succeeded in source and disposable-package validation; not yet a public release**
- Rollback: **not applicable; no ESO bundle, cache, or setting was changed**

## Question

Can normal operational logging and bounded incident diagnostics be selected
without repeatedly installing or removing the bridge, while preserving the
production performance, privacy, and exact-target safety boundaries?

## Design

The bridge retains `info` as its normal production default. When it starts, it
first honours the source-maintenance `TESO4M4_LOG_LEVEL` environment override.
Without that override, it reads only a one-line `level=error|warn|info|debug`
file in the existing per-installation Application Support state directory. The
file identity derives from the canonical `eso.app` bridge location, matching
the release tool's installation-state identity.

The parser follows fail-safe rules: the file must be a regular, current-user
file that is not group- or world-writable; symlinks, malformed content,
multiple lines, and `trace` are rejected. Missing or invalid state falls back
to `info`. Source-only `trace` remains available through the environment
because it can carry pointer-bearing, per-frame, or per-draw data and can
materially change timing.

The visible `Logging.command` verifies the exact executable, active bridge,
production marker, and recovery record before it writes the preference. It
offers `info` (operation) and `debug` (one bounded incident capture); Escape
cancels without changing the preference. The command alters no game bundle,
cache, user setting, backup, or running process, and the selected level takes
effect only on the next user-controlled normal launch. Remove deletes this
patcher-owned preference after its verified restore completes.

## Validation

- A dedicated native log-config probe passed missing-default, accepted `debug`,
  rejected `trace`, and rejected multi-line state cases.
- A fresh bridge build passed its Bink re-export, Rosetta self-patch, inactive
  pacing, logging, lifecycle, render, compatibility, and MoltenVK probes.
- The disposable release installer fixture passed setting/status reporting,
  command-level `debug` then `info` changes, `trace` refusal, and removal of
  the patcher-owned preference.
- Package assembly and archive validation cover the new visible command,
  executability, checksum manifest, LF policy, and private-file exclusion.

No ESO, Steam, or launcher process was launched for this work. No public
package was installed and no gameplay result is claimed.

## Operational plan

1. Keep `info` selected in ordinary operation; it preserves the bounded
   production records used by Status without enabling hot-path trace detail.
2. On a specific recurrence, preserve the current run before retrying. Select
   `debug`, make at most the already-justified natural launch, and retain the
   run ID with the user's focus/FPS observation and relevant sleep/wake timing.
3. Use Diagnostics for its filtered report; it never exports raw trace data.
   Return immediately to `info` after the capture.
4. A new debug field requires a stated question, named owner, bounded event or
   count limit, privacy classification, and policy-probe coverage. Promote a
   field to `info` only with an operational need and a demonstrated absence of
   hot-path cost; otherwise remove it after the issue is resolved.

## Interpretation

This changes observability operations, not the active runtime-control profile.
It makes the post-sleep investigation easier to capture without attributing a
focus/FPS cure to logging or changing the production timing path by default.
The public 0.2.0 ZIP remains unchanged; availability requires a subsequent
verified release package and a one-time user installation of that package.
