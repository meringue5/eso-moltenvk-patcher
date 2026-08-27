# Production baseline

Promoted: 2026-08-01

Current maintenance baseline: 2026-08-27 stable startup checkpoint

`teso4m4` is a production runtime patch from this promotion point onward. The
project's prior work remains preserved as research and experiment history; it
is the evidence that led to this release baseline and must not be rewritten as
if it had been production at the time.

## Production baseline identity

| Component | Production baseline |
|---|---|
| Product | ESO MoltenVK Patcher 0.1.3 |
| Current exact ESO client | Steam macOS ESO 12.0.8, databuild `3288357` |
| Extended gameplay client | Steam macOS ESO 12.0.7, databuild `3281538` |
| Replacement runtime | Official MoltenVK 1.4.2 |
| Bridge profile | `startup-compositor-neutralize-pacing-release`; inactive pacing bypass and bounded compositor repair |
| Validated hardware | Apple M4 MacBook Air, through Rosetta |
| Gameplay evidence | Roughly 93 minutes of ordinary play at the observed 60 FPS VSync ceiling, including live graphics and resolution changes |
| Startup evidence | Exact packaged 0.1.3 candidate had no pink or low FPS; 79 bounded suppressions, ordinal-150 forwarding, and no pipeline timing or lifecycle errors |
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
Version 0.1.2 promotes the Experiment 0038 performance-first control after an
exact user launch separated normal FPS from the still-visible pink placeholder.
Version 0.1.3 promotes Experiments 0044-0046 after the measurement-stripped
package candidate passed its exact user launch and generation-aware recovery
fixture.

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
Experiment 0038 disables the cosmetic compositor neutralizer and all supporting
startup audits while retaining non-maximized compilation and bounded pipeline
timing. Its first user-controlled launch followed the normal renderer path and
normal FPS with pink visible. The result selects the 0.1.2 product tradeoff; it
does not prove long-term recurrence absence.
Experiment 0044 then validates the exact inactive 100-ms pacing bypass and
bounded 79-draw compositor repair twice, including one run that exercised the
inactive branch. Experiment 0045 removes diagnostic pipeline timing and post-
window lifecycle bookkeeping without changing that functional repair; its
exact 0.1.3 package candidate launched without pink or low FPS and passed all
bounded log gates. Experiment 0046 makes install and uninstall recovery aware
of the executable/original-loader generation and prevents obsolete backup
restoration across an update.

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
- Keep the normal-FPS startup path and verified bounded pink suppression as one
  target-specific release invariant. Fail open after ordinal 150 and do not
  reintroduce persistent diagnostic timing or lifecycle bookkeeping.
- Do not claim that shader or pipeline-cache warm-up causes the reported
  restart-dependent cold-start slowdown until a controlled comparison separates
  it from launcher lifetime and other startup resource state.
- Do not claim broader hardware or distribution support without matching
  evidence.
