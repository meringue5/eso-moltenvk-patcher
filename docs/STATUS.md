# Project status

Last updated: 2026-07-19

## Safety state

The experimental MoltenVK bridge is not validated for gameplay. Experiments
0001 and 0002 activated MoltenVK 1.4.1 and then crashed during early graphics
startup. Experiment 0002 added complete proc tracing and is documented in its
[experiment record](experiments/0002-live-check-proc-trace-startup.md).

Experiment 0004 was explicitly approved, rebuilt from source commit `e8ee4d2`,
installed in `live-check` mode, and launched by the user through Steam. It
successfully filtered `VK_EXT_hdr_metadata`, recorded a device created without
that extension, and still crashed immediately at the confirmed NULL HDR setter.
See its [completed record](experiments/0004-hdr-advertisement-filter-startup.md).

After preserving the failed Experiment 0004 checkpoint and re-verifying all 14
evidence-file checksums, restoration became technically necessary for the
Experiment 0005 clean rebuild. The user stopped Steam and the launcher, and
`scripts/restore.sh` ran at approximately 2026-07-19 22:40 KST. The
non-destructive `scripts/status.sh` check after restoration reports:

- the analyzed ESO executable still matches the fingerprint in the
  [target manifest](../config/targets-eso-2026-07-11.json);
- the active Bink loader is original and the bridge is inactive;
- the enable marker is absent;
- the active and pristine Bink SHA-256 values both equal
  `c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`.

This is a point-in-time observation, not a persistent guarantee. Run the status
check again before any work involving the game bundle. Inactive companion files
may exist beside the game executable; the status result above does not inventory
or remove them.

The user directed that rollback is not an operational goal. The failed state
was preserved until restoration became necessary for a clean rebuild. Its raw
evidence, displaced marker, and pipeline-cache state remain preserved; the
current original-loader state is a build prerequisite, not an assertion that
normal operation was the objective.

Experiment 0002 was explicitly approved, installed from source commit
`7a235dc`, run by the user, collected, and rolled back. Immediately after that
rollback, the then-active loader byte-matched the pristine backup. Raw evidence
and its checksum manifest remain under the ignored `artifacts/` directory; they
must not be committed.

Experiment 0003 retrospectively examined a user-controlled session lasting
about 2 hours 27 minutes. Its loaded-image list contains only the original Bink
UUID and no bridge or dynamic MoltenVK image. The session therefore used the
embedded MoltenVK 1.0.18 runtime; it is not evidence that the override works.
Its current settings, updated pipeline cache, and exit report are preserved as
an ignored baseline checkpoint. See the
[Experiment 0003 record](experiments/0003-original-runtime-long-session.md).

## Active blocker

Experiment 0004 falsified the device-advertisement-only hypothesis. The bridge
removed exactly one HDR device extension, and ESO still queried and called the
NULL setter. In both Experiments 0002 and 0004, Rosetta `tmp1` equals the
ASLR-adjusted instruction immediately after ESO's indirect
`vkSetHdrMetadataEXT` call, confirming that call as the fault site.

The actual branch input is now identified. ESO enables its HDR surface flag
when surface enumeration includes format `64` with color space `1000104008`.
Embedded MoltenVK 1.0.18 reports three sRGB formats and no matching pair;
MoltenVK 1.4.1 reports 60 formats and includes exactly one. A source-only
wrapper removes that pair, produces 59 visible formats, and passes fake and real
non-game enumeration validation. The clean full build, legacy/raw/filtered
surface probes, device/proc probes, and static binary analyzers now pass. It has
not been installed or run with ESO.

MoltenVK 1.4.1 performance A/B testing remains blocked until a startup
experiment is stable.

The Experiment 0003 process ended with `EXC_BAD_INSTRUCTION / SIGILL` in an
audio teardown thread after otherwise usable gameplay. This is a separate
shutdown failure from the bridge startup crash.

## Performance baseline

The user reported generally higher FPS in Experiment 0003, object-heavy areas
dropping into the 40s, no visible improvement from changing graphics options in
that state, and occasional recovery to about 60 FPS without logout. These are
valuable observations but not a controlled old/new A/B: no paired GPU-time,
memory, thermal, camera, population, or before-session cache measurements were
captured. They must not be attributed to MoltenVK 1.4.1.

Rust tooling is now locally available as `rustc 1.97.1` and `cargo 1.97.1`,
with both Apple Silicon and x86_64 macOS targets installed. No project component
depends on Rust yet.

## Next gate

The Experiment 0005 non-game preflight and static checks are complete. Its clean
source state and fresh ignored evidence are prepared as the exact pre-install
checkpoint. The next gate is explicit user approval for the exact Experiment
0005 installation; Experiment 0004 approval does not carry over.

If separately approved and installed, the sole user action is limited to a
Steam-authenticated launch to character selection plus a 60-second wait. No
world entry, Metal HUD, screenshot, settings change, or performance measurement
is part of that run.
