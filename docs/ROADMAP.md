# Roadmap

This roadmap contains future work only. Current verified state is in
[Project status](STATUS.md); completed work remains in the
[experiment index](experiments/README.md).

## P0: make the first post-install start reliable

- Treat Experiment 0035's back-to-back pair as the current discriminator: both
  starts activated the full bridge, but only the smooth restart engaged
  `MTLCompilerService` and completed Metal compilation jobs.
- Treat Experiment 0036 as a failed fix: its process-unique canary reached the
  compiler service and completed both immediate jobs in a later low-FPS
  process, so readiness alone does not trigger ESO's normal pipeline path.
- Add low-overhead timing and bounded counters around the first
  `vkCreateGraphicsPipelines` wave and correlate call entry, return, duration,
  and pipeline count with compiler-service connection timing.
- Repair the missing readiness-success production log record before reusing
  that canary as a diagnostic invariant; do not retain it as a claimed fix.
- Classify each future start as bridge inactive, bridge active with the normal
  compiler path absent, or bridge active with compiler engagement.
- Snapshot pipeline-cache identity, size, and modification time around every
  start, preserve each naturally occurring generation before the next retry,
  and separate ESO restart effects from launcher restart effects. Two
  consecutive low-FPS exits already rewrote the same-size active cache to
  different hashes without normal ESO compilation.
- Capture fixed-scene FPS, GPU time, frame interval, app/Metal memory, and
  thermal state for the first bad and first clean starts.
- Treat failure to initiate the normal pipeline-compilation path as the leading
  hypothesis while retaining cache revalidation and other startup resource
  state as alternatives; do not delete or replace caches to force a result.
- Add a prelaunch preparation step only if it can be proven safe without
  starting ESO, bypassing authentication, or distributing proprietary cache
  data.

## P1: preserve the 1.4.2 production release

- Keep official MoltenVK 1.4.2, the validated performance configuration, and
  the exact bounded startup compositor neutralizer as one release baseline.
- Preserve the pristine loader, release restore record, historical runtime
  backups, all pipeline-cache generations, and the 48-key sanitized settings
  template with validated High subsampling.
- Keep release install and uninstall transactional, idempotent, and fail-closed
  on unknown files or incomplete recovery state.
- Treat regressions in ordinary play, live graphics resets, startup color, or
  restore behavior as release blockers.

## P2: maintain ESO update support

- Run the quick update gate after every ESO or launcher content update.
- Allow the packaged auditor to re-attest relocation-only executable updates
  only when embedded MoltenVK, patch sites, reference shape, proc routes, and
  runtime boundaries remain compatible; any mismatch requires manual analysis.
- Add a new supported target only after rebuild, non-game probes, disposable
  install/remove testing, and a user-controlled normal-launch smoke test.
- Never turn an unknown build into best-effort compatibility.

## P3: broaden verified compatibility

- Validate the same exact client through a direct ZeniMax installation when a
  suitable user-controlled test is available.
- Add Apple GPU and macOS versions only with explicit hardware/runtime evidence.
- Keep Steam and launcher discovery edition-neutral; authentication remains the
  responsibility of the user's normal launcher.

## P4: improve distribution

- Collect feedback on the unsigned ZIP's Terminal and Gatekeeper experience.
- Consider a signed/notarized app or DMG if Apple Developer membership becomes
  worthwhile.
- Add release automation only if it preserves exact payload hashes, archive
  hygiene, release notes, and rollback verification.

## Deferred performance work

- Do not change the production profile merely for speculative FPS gains.
- Reopen shader compression or other MoltenVK tuning only with a bounded
  non-game benchmark, a clean A/B design, and unchanged compatibility gates.
- Do not repeat failed 1.4.1-era experiments unless a new regression provides a
  specific reason and new discriminating evidence.
