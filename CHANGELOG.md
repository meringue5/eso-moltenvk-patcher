# Changelog

## 0.2.0 - 2026-08-27

- Productizes the architecture established by the host-loop and compositor
  analysis. Public `Status.command` verifies package, client, bridge, recovery
  generation, settings profile, and the latest bounded 79/150/180 startup run
  without launching or changing the game.
- Adds `Diagnostics.command`, which exports a privacy-filtered local support ZIP
  containing checksums, state summaries, and only allowlisted latest-run events.
  It excludes settings, caches, credentials, game files, home paths, pointer-
  bearing traces, and unrelated system logs.
- Verifies the package checksum manifest before every action, exposes a
  versioned `balanced-m4-1920x1200-v1` settings identity, and preserves the
  existing transaction-safe apply, upgrade, customization, and removal rules.
- Bounds production logs to the current 1 MiB file plus one rotated generation
  with owner-only permissions. The 79 repetitive per-draw suppression records
  move from info to debug while begin, latch, finish, mode, pacing, and safety
  evidence remain available to Status.
- Passes an exact final-bridge user launch with normal focus, no pink, normal
  perceived FPS, all 17 redirects, exact 79/150/180 startup control, and no
  bridge errors.
- Rejects the Metal argument-buffer performance candidate after three
  consecutive starts failed to capture mouse focus. The restored argument-
  buffers-off control captured focus normally and completed approximately 54
  minutes of ordinary play without a bridge error.
- Fixes the 0.2.0 M4 standard template at the user's selected balanced
  1920 x 1200 profile: subsampling 1, character resolution 2, shadows 1,
  planar water reflections 0, particle density 1, SSAO, high-resolution
  shadows, view distance 1.15, and VSync.
- Leaves the published 0.1.3 asset and its historical settings checkpoint
  immutable.

## 0.1.3 - 2026-08-27

- Promotes the strongest startup-reliability profile validated so far on the
  exact M4 and ESO 12.0.8 target. It combines the exact inactive 100-ms host-
  pacing bypass with the bounded compositor repair that suppresses only the 79
  verified pink-placeholder draws and forwards the normal scene at ordinal
  150.
- Removes first-64 graphics-pipeline timing from the release profile and makes
  retained lifecycle wrappers direct-forward without table bookkeeping after
  the bounded startup window. Functional safety evidence remains, while
  diagnostic work no longer persists into ordinary gameplay.
- The exact packaged candidate passed its user-controlled Steam launch with no
  pink and normal FPS. Its bounded log recorded exactly 79 suppressions,
  ordinal-150 forwarding, ordinal-180 completion, zero pipeline-timing
  records, and zero lifecycle errors or overflows. A short battery-mode mid-
  high observation also remained smooth, but is not presented as a sustained
  battery or cross-hardware benchmark.
- Makes Install, Status, and Uninstall original-loader-generation aware. If an
  ESO update leaves the bridge active, the patcher fails closed and requires
  launcher Repair instead of restoring a possibly obsolete Bink backup.
- Preserves a different launcher-provided original Bink byte-for-byte during
  Uninstall. A newer patcher may adopt that generation only after explicitly
  recognizing it, archiving the prior recovery generation, and creating a new
  verified backup.
- Extends the disposable release transaction fixture across retained-bridge
  refusal, externally restored original preservation, generation rotation,
  and interrupted archive resumption.
- Retains official MoltenVK 1.4.2, the exact ESO 12.0.8 target and structural
  update auditor, normal Steam/ZeniMax authentication, existing cache state,
  verified reversibility, and the optional 48-key M4 settings merge.

## 0.1.2 - 2026-08-25

- Prioritized startup FPS reliability over cosmetic pink-screen suppression.
  The release now uses the exact Experiment 0038
  `startup-pipeline-timing-control` profile: the compositor neutralizer and
  its supporting startup audits are disabled, while official MoltenVK 1.4.2,
  compatibility filters, asynchronous submission, disabled live-resource
  checks, and non-maximized pipeline compilation remain unchanged.
- Recorded a user-controlled launch with the intended separation: the original
  pink placeholder remained visible, FPS was normal, ESO issued its bulk
  pipeline wave at 11.652 seconds, and `RENDERER Complete` followed character
  selection by 2.685 seconds. All 64 retained pipeline calls succeeded, with a
  1.547 ms maximum duration.
- Preserved the pipeline cache produced by the preceding low-FPS run and still
  obtained the normal path, weakening permanent cache poisoning as a sufficient
  cause. No cache reset is part of the release.
- Kept bounded first-64-call pipeline timing for support evidence and added a
  release transaction assertion for the exact performance-first marker.
- Retained ESO 12.0.8 support, compatible-update re-attestation, verified
  backup/restore behavior, normal Steam/ZeniMax authentication, and the
  optional 48-key settings merge from 0.1.1.
- Documented the visible pink startup interval as a known cosmetic issue. One
  normal control is not claimed as proof of long-term reliability or of the
  exact internal ESO timing race.

## 0.1.1 - 2026-08-11

- Added the verified ESO 12.0.8/databuild `3288357` target after confirming
  that the embedded MoltenVK runtime and all production redirect boundaries
  remain compatible.
- Replaced absolute source-address equality in the update fast path with
  semantic reference and proc-query comparison, while retaining exact runtime,
  symbol, count, route, patch-byte, and replacement-runtime gates.
- Bundled a native relocation-tolerant compatibility auditor so the same
  installer can accept later ESO updates only when the embedded MoltenVK and
  complete bridge-facing structure remain unchanged.
- Attest the actual audited ESO SHA-256 in the install marker and verify that
  attestation again inside the runtime before applying any in-memory patch.
- Recover after launcher updates whether they restore the original Bink loader
  or leave the prior bridge loader installed, preserving the verified backup
  and historical state before reinstalling.
- Extended the release transaction fixture to cover both update-recovery
  states and added a negative changed-patch-byte auditor check.
- Recorded successful Steam-path activation and gameplay on ESO 12.0.8. A
  recurring first-one-or-two-start pink/approximately-10-FPS condition remains
  an explicit cold-start reliability follow-up; no shader-cache cause is yet
  claimed.

## 0.1.0 - 2026-08-02

- Rebuilt the release from the cleaned 1.4.2 production source after an exact
  installed-binary gameplay validation.
- Raised the bundled standard profile's subsampling setting from Medium to
  High after successful ordinary-play validation.
- Reduced the README installation section to a release-oriented quick start.
- Removed the abandoned MoltenVK 1.4.1 texture-cache backport, its dedicated
  build and comparison probes, and the failed legacy feature-profile mode.
- Reframed current status, roadmap, and architecture documentation around the
  1.4.2 production baseline while preserving experiment and research history.
- Promoted the user-installed `0.1.0-rc.1` package after successful ordinary
  Steam-path startup and gameplay on the supported M4/ESO 12.0.7 baseline.

- Replaced text confirmation prompts with dependency-free arrow-key menus that
  initially highlight the affirmative choice, retain Y/N shortcuts, confirm
  with Return, and cancel on Escape.
- Refined the interactive stepper to clear the shell's echoed launch command,
  show every stage up front in muted text, and promote completed stages to
  bright green checked rows while leaving redirected logs untouched.
- Allowed migration from the verified source-maintenance restore state when
  its inactive retagged-Bink and MoltenVK companions match exact known hashes;
  unknown companions still fail closed.
- Corrected the release profile's original-Bink fingerprint, which had
  accidentally recorded a built proxy hash; the fail-closed installer now
  accepts only the independently preserved original loader hash.
- Added a dependency-free terminal stepper with numbered real-work stages,
  explicit settings choices, safe failure messaging, and concise install and
  uninstall summaries; redirected output remains plain text.
- Simplified the ZIP's Finder view to `Install.command`,
  `Uninstall.command`, and `README.txt`, with payloads, checksums, and support
  diagnostics retained under a hidden internal directory.
- Bundled the 48-key M4 settings template with an explicit apply-or-skip
  installation choice, selective merge, verified backup, and conflict-safe
  removal behavior. Interactive installs highlight Apply initially, while
  non-interactive installs must state their choice.
- Adopted **ESO MoltenVK Patcher** as the canonical product name while
  retaining `teso4m4` installation and source identifiers for backward
  compatibility.
- Renamed the GitHub repository to `eso-moltenvk-patcher` and updated public
  download and support links to its canonical URL.
- Added a prominent latest-release download link and an illustrated end-user
  installation guide covering install, custom paths, Gatekeeper, and removal.
- Added the no-membership public release path: a prebuilt GitHub Release ZIP
  with one-step exact-profile `install` and `remove` commands, optional status,
  automatic Steam/ZeniMax path discovery, explicit custom-path support, and SHA-256
  checksums. Players do not need Python or Xcode. Interactive path fallback,
  durable release-version state, clear install/backup summaries, LF and archive
  hygiene gates, and a disposable install/remove/reinstall test cover the
  first-run experience. Transaction phases are journaled so a repeated Install
  verifies, rolls back, and cleanly restarts an interrupted installation.
- Added the native **ESO MoltenVK Patcher.app** release candidate, with
  client discovery, exact-profile validation, backup, install, repair, remove,
  and reproducible DMG assembly.
- Added production log levels and moved routine bridge logs to the user's
  Library Logs directory; detailed Vulkan tracing is opt-in.
- Promoted the verified MoltenVK 1.4.2 `performance-aggressive` checkpoint to
  **Production Baseline 2026-08-01** for Steam macOS ESO 12.0.7, databuild
  `3281538`.
- Added the production-scope record and separated the supported production
  baseline from the preserved pre-promotion research history.
- Added a fail-closed local ESO update checker with current-target selection.
- Added an exact static-profile audit for unchanged-layout target rebases.
- Recorded patch-site bytes, embedded archive members, Vulkan reference shape,
  proc-query shape, and replacement-runtime identity in the current manifest.
- Integrated pre/post update identity checks with experiment evidence handling.
- Added a guarded, reversible `SkipPregameVideos` settings helper.
- Made settings helpers fail closed when ESO process inspection is unavailable.
- Validated the official MoltenVK 1.4.2 `performance-aggressive` checkpoint in
  roughly 93 minutes of ordinary play, including six successful live graphics
  and resolution reset sequences at the observed 60 FPS VSync ceiling.
- Replaced the blanket Steam-closed maintenance condition with a shared
  bundle-idle gate that permits idle Steam while blocking actual ESO, launcher,
  file-use, update, or indeterminate activity.
- Established the **2026-08-02 startup-clean maintenance baseline**: two
  consecutive exact starts suppressed only the proven placeholder-compositor
  window, forwarded the normal scene at ordinal 150, and showed no pink frame,
  crash, settings change, or readback/queue-wait path.

## Pre-production baseline - 2026-07-19

- Documented reproducible FPS degradation and Metal HUD evidence.
- Recorded validated and rejected ESO settings advice.
- Identified statically linked MoltenVK 1.0.18.
- Added MoltenVK 1.4.1 compatibility probes.
- Added an x86_64 Bink re-export proxy and Mach VM patch prototype.
- Recorded the failed 17-entry-point redirect experiment and successful restore.
- Added sanitized null-function-pointer crash forensics and GIPA result tracing.
- Added guarded build, install, status, and recovery scripts.
