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

At 2026-07-19 22:54 KST, the user approved continuing directly with the
prepared Experiment 0005 installation. Pre-install checks found no ESO, Steam,
or launcher process; required original/inactive status and exact source commit
`b00ced5d521c301a1bc159b74cfa16056a5c5e36`; and reverified both prepared
artifact hashes. The bridge was then installed in `live-check` mode. Immediate
post-install status reports the bridge installed and marker present. The active
proxy and MoltenVK hashes match the prepared evidence, the pristine Bink remains
unchanged, and the 6,800,792-byte old pipeline cache is preserved under its
backup name.

The user ran Experiment 0005 through the normal Steam path from 23:04:28 to
23:05:44 KST, about 76 seconds. The automatic bridge verdict passed and no new
crash report was created. The run reached character selection and exited
normally, but rendering correctness failed visibly: startup briefly showed a
full-screen hot-pink frame, and character selection showed high-frequency
flicker in a black layer described as shadow-like. The installed checkpoint is
being preserved for analysis; it is not approved for world entry or gameplay.

After all 16 Experiment 0005 evidence checksums were reverified, restoration
became technically necessary for the Experiment 0006 source rebuild. At
approximately 23:37 KST, `scripts/restore.sh` restored the pristine loader and
the prior pipeline cache. The Experiment 0005 marker and its new cache remain
preserved under timestamped names. Post-restore status reports the exact ESO
fingerprint, original/inactive loader, absent active marker, and byte-identical
active/pristine Bink files. This is a build checkpoint, not an operational
rollback objective.

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

The former startup blocker is resolved for one controlled run. ESO enables its
HDR surface flag when surface enumeration includes format `64` with color space
`1000104008`.
Embedded MoltenVK 1.0.18 reports three sRGB formats and no matching pair;
MoltenVK 1.4.1 reports 60 formats and includes exactly one. A source-only
wrapper removed that pair from both ESO count and data queries, produced 59
visible formats, and prevented any `vkSetHdrMetadataEXT` lookup. ESO continued
through swapchain, pipeline, draw, present, and orderly teardown calls.

The active blocker is now rendering correctness. The user observed a transient
hot-pink full screen and persistent high-frequency black/shadow-layer flicker
at character selection. The unified log contains 106 privacy-redacted Metal
compiler warnings between 23:04:34 and 23:05:08 KST, but neither that timing nor
the hidden messages prove the cause. ESO's own short logs report renderer and
texture completion without a relevant error.

MoltenVK 1.4.1 performance A/B testing remains blocked until rendering
correctness and short gameplay stability are established.

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

Experiment 0006 is the next single-variable candidate. It keeps both proven HDR
filters and live-resource checking but disables Metal argument buffers.
MoltenVK 1.0.18 predates argument-buffer support, MoltenVK 1.4.1 enables it by
default, and ESO enabled no descriptor-indexing extension in the captured
device. The bridge will query and validate the effective configuration before
writing any patch.

The rebuilt candidate has passed configuration, Vulkan/Metal, HDR filter,
wrapper coverage, proc route, Python, and shell checks. In independent
processes the packaged runtime reported argument buffers `1` under controlled
defaults and `0` in `descriptor-compat`, with all other named controls
unchanged.

Source commit `b2817da` was recorded in the ignored evidence directory
`artifacts/experiment-0006-20260719T144353Z/`. With no ESO, Steam, or launcher
process present, the matching artifacts were installed at approximately 23:44
KST. Immediate status reports an installed bridge and a marker containing
`descriptor-compat`; active proxy and MoltenVK hashes match the prepared
evidence, the pristine Bink is unchanged, and both older cache generations are
preserved. No agent launched the game.

The installed checkpoint is ready for one staged run: observe character
selection for 30 seconds; if it is visually clean, continue in the same run to
five minutes of low-risk world movement. If the black/shadow-layer flicker
remains, exit without world entry. This is a correctness and short-stability
gate, not a performance test; no HUD, capture, or settings change is requested.
