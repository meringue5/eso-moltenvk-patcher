# Experiment 0032: 0.1.0 release-candidate end-to-end validation

- Date: 2026-08-02
- Outcome: **succeeded; promoted to 0.1.0**
- Rollback: **not required; validated package remains installed**

## Question

Does the player-facing `0.1.0-rc.1` ZIP complete the real install-to-gameplay
path on the supported production target without the source checkout, Python,
Xcode, or manual agent installation?

## Target and package

- ESO macOS client 12.0.7, databuild `3281538`.
- Exact executable SHA-256:
  `82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`.
- Official MoltenVK 1.4.2 runtime SHA-256:
  `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
- Player-facing package version recorded by its installation state:
  `0.1.0-rc.1`.
- Normal Steam launcher and authentication path; the agent did not launch ESO,
  Steam, or the ZeniMax launcher.

## Procedure

1. The user opened the locally assembled public ZIP and ran `Install.command`.
2. The installer discovered and verified the supported ESO application, made
   its restorable installation state, and installed the packaged bridge and
   runtime.
3. The user launched ESO through the normal Steam path and performed ordinary
   gameplay.
4. The user reported the result; the agent then inspected only the sanitized
   production bridge log and non-destructive status output.

## Evidence

The installation state records release version `0.1.0-rc.1`. Post-play status
reports the bridge installed against the current exact ESO target, the enable
marker present, official MoltenVK current, and all three preserved pipeline
cache identities valid.

The production-info run is
`20260802T084433.627791000Z-pid88181`. Its retained events establish:

```text
log level:          info
runtime:            official MoltenVK 1.4.2
redirects:          17 Vulkan entry points active
startup strategy:   ordinal-window, fail-open
suppressed draws:   79
forward latch:      generation 2, ordinal 150, present-deadline
pixel readback:     disabled
```

The research startup checker expects diagnostic GIPA, device-creation,
surface-format, and audit-arm events that production info logging deliberately
omits. Its failure on this reduced log is therefore not treated as a runtime
failure or substituted for the exact retained production events. This record
does not claim those omitted events were independently re-observed.

The user reported that startup and actual gameplay completed without a
problem. No continuous frame-rate measurement or fixed gameplay duration was
requested for this release usability validation.

## Result

The complete public-package path succeeded: archive extraction, interactive
installation, normal authenticated launch, and ordinary gameplay. The bridge
remains installed on the exact current target with a recorded restore path.

## Interpretation

Confirmed: the release ZIP is usable without development dependencies and the
packaged production bridge runs successfully on the supported M4/ESO 12.0.7
baseline. Together with the earlier extended gameplay and two-start mechanism
evidence, this closes the final release-candidate usability gate.

This is not evidence for untested hardware, unknown ESO builds, or a different
macOS client identity. Those boundaries remain fail-closed.

## Follow-up

Promote the same source and payload to semantic version `0.1.0`, rebuild the
versioned ZIP, repeat non-game release gates, publish its SHA-256, and retain
the installed RC state until the user chooses to upgrade or uninstall it.
