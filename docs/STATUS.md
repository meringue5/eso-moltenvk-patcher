# Project status

Last updated: 2026-07-19

## Safety state

The experimental MoltenVK bridge is not validated for gameplay. Experiments
0001 and 0002 activated MoltenVK 1.4.1 and then crashed during early graphics
startup. Experiment 0002 added complete proc tracing and is documented in its
[experiment record](experiments/0002-live-check-proc-trace-startup.md).

The non-destructive `scripts/status.sh` check after Experiment 0003 on
2026-07-19 reported:

- the analyzed ESO executable still matches the fingerprint in the
  [target manifest](../config/targets-eso-2026-07-11.json);
- the active Bink loader is original and the bridge is inactive;
- the enable marker is absent.

This is a point-in-time observation, not a persistent guarantee. Run the status
check again before any work involving the game bundle. Inactive companion files
may exist beside the game executable; the status result above does not inventory
or remove them.

Experiment 0002 was explicitly approved, installed from source commit
`7a235dc`, run by the user, collected, and rolled back. The active loader
byte-matches the pristine backup. Raw evidence and its checksum manifest remain
under the ignored `artifacts/` directory; they must not be committed.

Experiment 0003 retrospectively examined a user-controlled session lasting
about 2 hours 27 minutes. Its loaded-image list contains only the original Bink
UUID and no bridge or dynamic MoltenVK image. The session therefore used the
embedded MoltenVK 1.0.18 runtime; it is not evidence that the override works.
Its current settings, updated pipeline cache, and exit report are preserved as
an ignored baseline checkpoint. See the
[Experiment 0003 record](experiments/0003-original-runtime-long-session.md).

## Active blocker

Experiment 0002 reproduced `EXC_BAD_ACCESS` with `RIP=0` and the same first
recoverable ESO return offset as Experiment 0001. The last recorded proc lookup
was a NULL GDPA result for `vkSetHdrMetadataEXT`. MoltenVK 1.0.18 does not
advertise `VK_EXT_hdr_metadata`; MoltenVK 1.4.1 advertises it but returns a NULL
device proc unless that extension was enabled when creating the device.

An extension-negotiation mismatch is now the leading explanation, but it is not
confirmed. The bridge did not record the extension list passed to
`vkCreateDevice`, and the crash report cannot prove that ESO called the last
logged null pointer. MoltenVK 1.4.1 performance A/B testing remains blocked.

The Experiment 0004 build now filters only the HDR advertisement and records
the exact device-extension list. Its shared fake-runtime tests and real
MoltenVK 1.4.1 non-game probe pass with raw HDR advertisement present, visible
advertisement absent, HDR disabled, non-null GIPA, and NULL GDPA. This is
evidence that the compatibility transition works locally; it does not confirm
the proposed crash cause in ESO.

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

All agent-owned pre-installation gates for the
[Experiment 0004 plan](experiments/0004-hdr-advertisement-filter-startup.md)
have passed. The game remains original and unmodified. The next gate is the
user's explicit approval to install that exact startup-only experiment; the
current goal and prior approvals do not imply it.

After an approved install, the sole user action will be a Steam-authenticated
launch to character selection, a 60-second wait there, and exit: approximately
3-5 minutes total. There is no world entry, Metal HUD, screenshot, settings
change, or performance measurement. Evidence collection applies an automatic
run-scoped verdict, and the bridge is restored immediately regardless of the
outcome.

Performance investigation is deferred until a startup experiment proves that
MoltenVK 1.4.1 remains active and stable. Do not repeat either failed
installation unchanged.
