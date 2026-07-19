# Experiment 0004: HDR-advertisement-filtered startup

- Date planned: 2026-07-19
- Outcome: **planned; awaiting explicit installation approval**
- Rollback: **not started; pristine restore path verified**

## Question

Does presenting MoltenVK 1.4.1 without its newly advertised
`VK_EXT_hdr_metadata` device extension keep ESO on the embedded 1.0.18
extension-negotiation path and allow startup to remain stable at character
selection?

## Hypothesis

Experiment 0002 reached swapchain setup, queried `vkSetHdrMetadataEXT` through
GDPA, received NULL, and then reproduced the same `RIP=0` crash as Experiment
0001. The new runtime advertises the HDR extension, while embedded MoltenVK
1.0.18 does not.

If the new advertisement causes ESO to select the unsafe path, filtering only
`VK_EXT_hdr_metadata` from device-extension enumeration will produce all of the
following:

- the raw 1.4.1 list contains the extension, but ESO's visible list does not;
- `vkCreateDevice` does not enable the extension;
- ESO does not query `vkSetHdrMetadataEXT` through GDPA;
- the process reaches character selection and remains there for 60 seconds.

A repeated HDR proc query, startup crash, or different enabled-extension state
falsifies or weakens this hypothesis and returns the project to non-game
analysis. A guarded no-op setter is not part of this experiment.

## Target and change set

- ESO SHA-256:
  `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`
- ESO Mach-O UUID: `867e93bc-a6e7-3109-bf8e-542ff59ccdff`
- Runtime: official pinned MoltenVK 1.4.1 release
- Redirect set: the same 17 byte-validated wrapper entries as Experiments 0001
  and 0002
- Marker mode: `live-check`, matching Experiment 0002 so the HDR filter is the
  relevant behavioral change
- Source identity: the commit containing this plan; record its exact hash in
  prepared evidence before installation
- Installation state at planning: original Bink active, bridge inactive,
  enable marker absent

The bridge now returns wrappers for GIPA requests for
`vkEnumerateDeviceExtensionProperties` and `vkCreateDevice`. The enumeration
wrapper removes only `VK_EXT_hdr_metadata`, preserves every other property and
its order, and implements Vulkan count/data and `VK_INCOMPLETE` behavior. The
device wrapper does not alter `VkDeviceCreateInfo`; it records the exact enabled
extension names and forwards the call.

Every bridge line has a UTC nanosecond run timestamp and PID. Each device-create
request, indexed extension name, and result also shares a monotonic call ID.
The evidence checker groups by run ID, verifies every declared extension index,
and will not accept an active record older than the prepared experiment
timestamp.

## Completed agent-only preflight

- `scripts/status.sh` confirmed the fingerprinted ESO executable, original
  loader, inactive bridge, and absent marker.
- The shared fake-runtime smoke probe verified:
  - count-only enumeration removes exactly one HDR property;
  - exact-capacity enumeration preserves the other properties and order;
  - short-capacity enumeration returns `VK_INCOMPLETE` without exposing HDR;
  - layer-specific enumeration passes through unchanged;
  - `vkCreateDevice` requests, including explicit HDR requests, are logged and
    forwarded unchanged.
- The real MoltenVK 1.4.1 non-game probe on the Apple M4 reported:

  ```text
  raw device extension VK_EXT_hdr_metadata       yes
  visible extension  VK_EXT_hdr_metadata         NO
  probe enable      VK_EXT_hdr_metadata          no
  hdr negotiation raw-advertised=yes visible=no enabled=no GIPA=yes GDPA=NULL
  hdr filter validation: PASS
  ```

- Mach-O wrapper analysis recovered 162 old Vulkan text symbols, 40 external
  references, and exactly 17 externally referenced entry points. Manifest
  omissions: 0; unreferenced manifest entries: 0; referenced entries absent
  from MoltenVK 1.4.1: 0.
- Proc-query analysis recovered 19 GIPA sites with 17 unique names and 80 GDPA
  sites with 65 unique names. Unknown sites: 0; probe candidates missing: 0;
  old-nonnull/new-null regressions on ESO's actual routes: 0.
- A source rebuild passed the Bink re-export check, Rosetta self-patch
  transition `1 -> 2`, and the HDR filter smoke probe.
- Startup-log checker tests cover a complete pass, stale and invalid run-ID
  rejection, newest-run selection, HDR proc-query rejection, HDR-enabled-device
  rejection, missing-filter rejection, and incomplete device-extension-list
  rejection.
- A no-launch evidence-collection smoke run produced `startup bridge verdict:
  FAIL` with `no active instrumented run matched the time gate`, exit code 2,
  zero new crash reports, and a checksum manifest that passed verification.

These checks prove the filter and diagnostic behavior outside ESO. They do not
prove ESO will follow the predicted branch or remain stable; that is the single
question authorized for the eventual startup run.

## Installation gate

This goal does not authorize installation. Before any modification:

1. Obtain explicit user approval for this exact Experiment 0004 installation.
2. Confirm ESO, Steam, and the launcher are stopped.
3. Re-run `scripts/status.sh`; require the original loader and absent marker.
4. Rebuild from the approved source commit; do not use stale `build/` output.
5. Verify the active Bink byte-matches the pristine restore source, and verify
   the restore script and old pipeline-cache preservation path.
6. Run `scripts/prepare-evidence.sh` before installation. It requires a clean
   source worktree and records the source commit plus hashes of the built proxy
   and MoltenVK copy in ignored evidence.
7. Install only through the experimental gate in `live-check` mode.

Any mismatch aborts before changing the game bundle.

## Exact user action

The user owns all interactive control:

1. Launch ESO through the normal Steam and launcher path.
2. If character selection appears, remain there for 60 seconds.
3. Exit through the normal UI and report only whether character selection was
   reached and remained stable for the full 60 seconds.

Do not enter the world. Do not enable Metal HUD, capture a screenshot, change a
setting, or perform performance testing. Maximum expected interaction is 3-5
minutes. Stop immediately at a crash, hang, visible corruption, or the end of
the 60-second wait.

Agents must not launch ESO, Steam, or the launcher.

## Evidence and automatic verdict

Immediately after the user stops, run `scripts/collect-evidence.sh` on the
prepared directory. It copies the accumulated bridge log, selects only an
instrumented active run newer than the preparation timestamp, captures new
`.ips` and unified logs, writes a startup verdict and exit code, captures final
status, and produces `SHA256SUMS`.

The bridge-log checker passes only if the selected run contains all of these:

- matching run timestamp and PID plus MoltenVK load and 17-entry activation;
- GIPA routing through the HDR enumeration filter and device-create tracer;
- count and data enumeration records each removing exactly one HDR extension;
- every observed `vkCreateDevice` has `hdr_enabled=no`, no HDR extension name,
  and a successful non-null result;
- no `vkSetHdrMetadataEXT` GDPA query and no bridge error/fatal record.

### Overall pass

- automatic bridge verdict is `PASS`;
- character selection remained stable for 60 seconds;
- no startup `RIP=0`/HDR-path crash occurred.

A shutdown-only `.ips` after the completed 60-second wait does not
automatically invalidate startup. It must be classified by timestamp and stack;
the known CoreAudio teardown `SIGILL` is recorded separately from the startup
outcome.

### Fail

- crash, hang, or corruption before completing the character-selection wait;
- automatic bridge verdict `FAIL`, including any HDR proc query or HDR-enabled
  device;
- the Experiment 0001/0002 `RIP=0` signature or another startup crash.

### Inconclusive

- no new instrumented active run, missing/corrupt evidence, or user stop before
  either a failure or the full stable wait.

No result authorizes world entry or a performance claim.

## Immediate rollback

After evidence collection, regardless of pass, fail, or inconclusive result:

1. Run `scripts/restore.sh` immediately.
2. Run `scripts/status.sh`; require original/inactive loader and absent marker.
3. Verify the active Bink byte-matches the pristine backup.
4. Verify the old pipeline cache was restored. Preserve any new experimental
   cache under the timestamped name chosen by the restore script.
5. Run `shasum -c SHA256SUMS` in the evidence directory.

Never delete the pristine backup, old cache, new experimental cache, crash
report, settings, or bridge log.

## Follow-up

If the experiment passes, promote only the startup compatibility result and
design a separate short stability experiment; do not begin performance testing
automatically. If it fails or is inconclusive, restore first and return to
non-game source, log, crash, and binary analysis before proposing another user
run.
