# Roadmap

This roadmap contains future work only. Current verified state is in
[Project status](STATUS.md); completed work remains in the
[experiment index](experiments/README.md).

## P0: isolate the 0.1.2 low-FPS release incident

- Treat Experiment 0040 as falsifying the no-neutralizer profile as a
  reliability repair. The exact 0.1.2 control reproduced pink and low FPS,
  delayed graphics-pipeline call 5 to 32.698 seconds, and delayed renderer
  completion to 13.762 seconds even though all retained calls succeeded
  quickly.
- Exclude compositor neutralization as a necessary cause and do not describe
  0.1.2 as a demonstrated low-FPS fix. Its packaging, exact-target, and
  reversibility claims remain independently verified.
- Treat the exact ESO inactive-loop path as the leading mechanism: when its
  internal application-active byte is false, `GameClient::mainLoop` calls
  `usleep(100000)`, directly imposing an approximately 10-Hz outer loop.
- Run one user-controlled ordinary Steam-authenticated launch of the installed
  Experiment 0041 candidate. Classify normal versus low FPS and correlate the
  observation with the bounded internal application-active state log. Pink
  alone remains an expected cosmetic result rather than a failure.
- Use a bounded, forward-only device/swapchain/queue/present trace only if the
  focused inactive-loop bypass does not eliminate low FPS. Do not re-enable
  the pink neutralizer in either diagnostic.

- Treat Experiment 0035's back-to-back pair as the current discriminator: both
  starts activated the full bridge, but only the smooth restart engaged
  `MTLCompilerService` and completed Metal compilation jobs.
- Treat Experiment 0036 as a failed fix: its process-unique canary reached the
  compiler service and completed both immediate jobs in a later low-FPS
  process, so readiness alone does not trigger ESO's normal pipeline path.
- Treat Experiment 0037 as a failed repair and successful diagnostic: low FPS
  recurred with 64/64 successful fast pipeline calls because ESO delayed
  issuing the bulk wave by about 20.8 seconds.
- Preserve Experiment 0038's first normal result: with neutralization and all
  supporting audits disabled, pink remained visible but FPS and renderer timing
  returned to the normal path.
- Continue classifying naturally occurring starts without forcing repeated
  gameplay; the low-FPS recurrence is now an open release incident.
- Repair the missing readiness-success production log record before reusing
  that canary as a diagnostic invariant; do not retain it as a claimed fix.
- Classify each future start from direct graphics-pipeline call/return timing
  and user-visible state. Treat compiler-service engagement only as supporting
  evidence: one normal process remained canary-only for more than nine minutes.
- Record ESO and launcher process identity for correlation, but do not make
  launcher restart a required workaround or causal assumption. One four-run
  sequence recovered after a launcher restart, while prior recovery did not
  always require it.
- Snapshot pipeline-cache identity, size, and modification time around every
  start, preserve each naturally occurring generation before the next retry,
  and separate ESO restart effects from launcher restart effects. Two
  consecutive low-FPS exits already rewrote the same-size active cache to
  different hashes without normal ESO compilation.
- Capture fixed-scene FPS, GPU time, frame interval, app/Metal memory, and
  thermal state for the first bad and first clean starts.
- Treat the alternate ESO renderer-initialization path as confirmed: low starts
  pass through the game-data/character-data waits and delay `RENDERER Complete`
  to about 13 seconds versus about 2.7 seconds in the preceding normal start.
  Maximum concurrent compilation has been falsified as a repair; retain cache
  revalidation and other startup resource state as alternatives and do not
  delete or replace caches to force a result.
- Add a prelaunch preparation step only if it can be proven safe without
  starting ESO, bypassing authentication, or distributing proprietary cache
  data.

## P1: isolate an optional pink repair

- Keep visible pink as an accepted cosmetic limitation in 0.1.2.
- Before any compositor substitution returns, build a forward-only control that
  retains equivalent tracking and locking without replacing draws. This must
  separate tracking overhead from the 79 draw-to-clear substitutions.
- Do not hold the completed 0.1.2 release branch open for this optional work;
  use a new branch and experiment only when the investigation resumes.

## P2: preserve the 1.4.2 production release

- Keep official MoltenVK 1.4.2, the validated performance configuration, and
  the exact no-neutralizer startup control as one release baseline.
- Preserve the pristine loader, release restore record, historical runtime
  backups, all pipeline-cache generations, and the 48-key sanitized settings
  template with validated High subsampling.
- Keep release install and uninstall transactional, idempotent, and fail-closed
  on unknown files or incomplete recovery state.
- Treat regressions in ordinary play, live graphics resets, startup color, or
  restore behavior as release blockers.

## P3: maintain ESO update support

- Run the quick update gate after every ESO or launcher content update.
- Allow the packaged auditor to re-attest relocation-only executable updates
  only when embedded MoltenVK, patch sites, reference shape, proc routes, and
  runtime boundaries remain compatible; any mismatch requires manual analysis.
- Add a new supported target only after rebuild, non-game probes, disposable
  install/remove testing, and a user-controlled normal-launch smoke test.
- Never turn an unknown build into best-effort compatibility.

## P4: broaden verified compatibility

- Validate the same exact client through a direct ZeniMax installation when a
  suitable user-controlled test is available.
- Add Apple GPU and macOS versions only with explicit hardware/runtime evidence.
- Keep Steam and launcher discovery edition-neutral; authentication remains the
  responsibility of the user's normal launcher.

## P5: improve distribution

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
