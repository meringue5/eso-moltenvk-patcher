# Changelog

## Unreleased

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

## 0.1.0 - 2026-07-19

- Documented reproducible FPS degradation and Metal HUD evidence.
- Recorded validated and rejected ESO settings advice.
- Identified statically linked MoltenVK 1.0.18.
- Added MoltenVK 1.4.1 compatibility probes.
- Added an x86_64 Bink re-export proxy and Mach VM patch prototype.
- Recorded the failed 17-entry-point redirect experiment and successful restore.
- Added sanitized null-function-pointer crash forensics and GIPA result tracing.
- Added guarded build, install, status, and recovery scripts.
