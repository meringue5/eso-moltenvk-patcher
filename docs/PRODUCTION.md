# Production baseline

Promoted: 2026-08-01

`teso4m4` is a production runtime patch from this promotion point onward. The
project's prior work remains preserved as research and experiment history; it
is the evidence that led to this release baseline and must not be rewritten as
if it had been production at the time.

## Production baseline identity

| Component | Production baseline |
|---|---|
| Product | `teso4m4` macOS runtime patch |
| ESO client | Steam macOS ESO 12.0.7, databuild `3281538` |
| Replacement runtime | Official MoltenVK 1.4.2 |
| Bridge profile | `performance-aggressive` |
| Validated hardware | Apple M4 MacBook Air, through Rosetta |
| Gameplay evidence | Roughly 93 minutes of ordinary play at the observed 60 FPS VSync ceiling, including live graphics and resolution changes |
| Safety boundary | Exact executable identity, static layout, original patch bytes, backup, and restore path must all pass |

No semantic product version has been assigned yet. Until the first GitHub
Release is cut, **Production Baseline 2026-08-01** is the public version name.

## Promotion boundary

The project was research-only through the controlled experiment sequence that
established compatibility, rendering correctness, and ordinary-play stability.
Experiment 0021's MoltenVK 1.4.2 maintenance adoption and the subsequent
ordinary-play validation establish this production baseline. Later startup
artifact investigations are bounded maintenance diagnostics; they do not
reclassify the working gameplay patch as experimental.

## Supported scope today

The production baseline is verified for the Steam macOS installation and its
normal authenticated launch path. The technical target is the exact `eso.app`,
not Steam itself. Direct ZeniMax-launcher support remains pending equivalent
client identity, install/restore, update, and normal-launch validation.

The current source scripts are developer and maintenance tooling, not the
end-user installer. Their explicit control gates remain in place while the
signed GitHub Release installer is designed and built.

An idle Steam client is not a dependency of bundle installation or
restoration and need not be closed. Maintenance tooling instead blocks when
ESO or the ZeniMax launcher is running, Steam reports ESO update/download
activity, any process holds a file inside the target `eso.app`, or one of those
checks is indeterminate. This keeps the production gate tied to the actual
bundle race rather than the presence of the authentication client.

## Product commitments

- Never patch an unrecognized client build.
- Preserve a verified restore path for each selected installation.
- Keep account login and game launch under the user's normal Steam or ZeniMax
  launcher flow.
- Treat the transient startup-magenta artifact as a maintenance defect, not a
  reason to downgrade the validated gameplay baseline.
- Do not claim broader hardware or distribution support without matching
  evidence.
