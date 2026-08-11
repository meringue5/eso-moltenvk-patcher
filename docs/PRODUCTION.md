# Production baseline

Promoted: 2026-08-01

Current maintenance baseline: 2026-08-11 update-compatible checkpoint

`teso4m4` is a production runtime patch from this promotion point onward. The
project's prior work remains preserved as research and experiment history; it
is the evidence that led to this release baseline and must not be rewritten as
if it had been production at the time.

## Production baseline identity

| Component | Production baseline |
|---|---|
| Product | ESO MoltenVK Patcher 0.1.1 |
| Current exact ESO client | Steam macOS ESO 12.0.8, databuild `3288357` |
| Extended gameplay client | Steam macOS ESO 12.0.7, databuild `3281538` |
| Replacement runtime | Official MoltenVK 1.4.2 |
| Bridge profile | `performance-aggressive` plus the bounded startup compositor neutralizer |
| Validated hardware | Apple M4 MacBook Air, through Rosetta |
| Gameplay evidence | Roughly 93 minutes of ordinary play at the observed 60 FPS VSync ceiling, including live graphics and resolution changes |
| Startup evidence | Two consecutive exact starts with 79 target draws neutralized at ordinals 71--149 and the normal scene forwarded at ordinal 150 |
| Standard settings | 2048 x 1280 profile with High subsampling (`SUB_SAMPLING "2"`) |
| Safety boundary | Exact selected identity or complete structural compatibility audit, original patch bytes, executable attestation, backup, and restore path must all pass |

The first semantic product version is **ESO MoltenVK Patcher 0.1.0**, promoted
on 2026-08-02 from the user-installed and gameplay-tested `0.1.0-rc.1` package,
then rebuilt from the cleaned production source after the exact successor
binary and High subsampling passed ordinary gameplay in Experiment 0033.
**Production Baseline 2026-08-01** remains the historical promotion boundary;
the 2026-08-02 startup-clean checkpoint is the maintenance baseline shipped in
0.1.0. Version 0.1.1 retains that runtime and promotes the 2026-08-11
update-compatible checkpoint after ESO 12.0.8 passed static, installer,
runtime-activation, and user-controlled gameplay gates.

## Promotion boundary

The project was research-only through the controlled experiment sequence that
established compatibility, rendering correctness, and ordinary-play stability.
Experiment 0021's MoltenVK 1.4.2 maintenance adoption and the subsequent
ordinary-play validation establish this production baseline. Later startup
artifact investigations are bounded maintenance diagnostics; they do not
reclassify the working gameplay patch as experimental. Experiment 0031's two
successful starts promote its exact bounded compositor substitution to the
current maintenance baseline. The 0.1.0 release package integrates that
behavior and passed an end-to-end user install, startup, and ordinary gameplay
validation as recorded in Experiment 0032. Experiment 0033 validates the exact
cleaned successor binary used for the replacement 0.1.0 payload.
Experiment 0034 validates recovery onto ESO 12.0.8 and the packaged
compatibility boundary used by 0.1.1. Its recurring first-one-or-two-start
performance observation remains a maintenance issue rather than a resolved
production claim.

## Supported scope today

The production gameplay baseline was observed through the Steam-authenticated
launch path. The technical target, and the release install criterion, is the
audited `eso.app` executable—not Steam itself. A direct ZeniMax-launcher
installation is eligible when the selected bundle passes the same exact or
structural-compatibility, restore, and bundle-idle checks; it keeps its normal ZeniMax
authentication and launch path. A direct-path gameplay run remains additional
compatibility evidence, rather than a prerequisite for safely recognizing an
identical client.

The current source scripts remain developer and maintenance tooling. The
end-user distribution is the prebuilt GitHub Release ZIP; its explicit
identity, backup, idle-bundle, and restore gates remain in place. A signed and
notarized app/DMG remains an optional future distribution path.

An idle Steam client is not a dependency of bundle installation or
restoration and need not be closed. Maintenance tooling instead blocks when
ESO or the ZeniMax launcher is running, Steam reports ESO update/download
activity, any process holds a file inside the target `eso.app`, or one of those
checks is indeterminate. This keeps the production gate tied to the actual
bundle race rather than the presence of the authentication client.

## Product commitments

- Never patch a client that is neither the exact selected target nor a complete
  match for the packaged structural compatibility fingerprint.
- Preserve a verified restore path for each selected installation.
- Keep account login and game launch under the user's normal Steam or ZeniMax
  launcher flow.
- Retain the validated bounded startup-magenta neutralizer in the release
  integration; it is a presentation repair and does not claim to modify ESO's
  underlying placeholder resource.
- Do not claim that shader or pipeline-cache warm-up causes the reported
  restart-dependent cold-start slowdown until a controlled comparison separates
  it from launcher lifetime and other startup resource state.
- Do not claim broader hardware or distribution support without matching
  evidence.
