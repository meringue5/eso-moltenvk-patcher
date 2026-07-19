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

The bridge correctness gate remains a non-game diagnostic that records the
enabled-extension list at `vkCreateDevice` and reproduces the new runtime's HDR
proc behavior for that exact list. Only after that evidence should another
startup experiment be designed, with an explicit choice between filtering the
newly advertised HDR extension and providing a guarded compatibility
implementation.

In parallel, the performance gate is a repeat of the original-runtime baseline
on a fixed route with paired Metal HUD captures before and after spontaneous
recovery. This does not require or authorize another bridge installation.

Do not repeat either failed installation unchanged. Any next bridge run remains
a startup evidence test, not a gameplay or performance test.
