# Project status

Last updated: 2026-07-19

## Safety state

The experimental MoltenVK bridge is not validated for gameplay. Experiments
0001 and 0002 activated MoltenVK 1.4.1 and then crashed during early graphics
startup. Experiment 0002 added complete proc tracing and is documented in its
[experiment record](experiments/0002-live-check-proc-trace-startup.md).

The non-destructive `scripts/status.sh` check on 2026-07-19 reported:

- the analyzed ESO executable still matches the fingerprint in the
  [target manifest](../config/targets-eso-2026-07-11.json);
- the active Bink loader is original and the bridge is inactive;
- the enable marker is absent;
- the old pipeline cache has been restored.

This is a point-in-time observation, not a persistent guarantee. Run the status
check again before any work involving the game bundle. Inactive companion files
may exist beside the game executable; the status result above does not inventory
or remove them.

Experiment 0002 was explicitly approved, installed from source commit
`7a235dc`, run by the user, collected, and rolled back. The active loader
byte-matches the pristine backup. Raw evidence and its checksum manifest remain
under the ignored `artifacts/` directory; they must not be committed.

## Active blocker

Experiment 0002 reproduced `EXC_BAD_ACCESS` with `RIP=0` and the same first
recoverable ESO return offset as Experiment 0001. The last recorded proc lookup
was a NULL GDPA result for `vkSetHdrMetadataEXT`. MoltenVK 1.0.18 does not
advertise `VK_EXT_hdr_metadata`; MoltenVK 1.4.1 advertises it but returns a NULL
device proc unless that extension was enabled when creating the device.

An extension-negotiation mismatch is now the leading explanation, but it is not
confirmed. The bridge did not record the extension list passed to
`vkCreateDevice`, and the crash report cannot prove that ESO called the last
logged null pointer. Performance testing remains blocked.

## Next gate

The next gate is a non-game diagnostic that records the enabled-extension list
at `vkCreateDevice` and reproduces the new runtime's HDR proc behavior for that
exact list. Only after that evidence should a third startup experiment be
designed, with an explicit choice between filtering the newly advertised HDR
extension and providing a guarded compatibility implementation.

Do not repeat either failed installation unchanged. Any third run remains a
startup evidence test, not a gameplay or performance test.
