# Changelog

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
