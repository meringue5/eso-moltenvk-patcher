# Experiment 0004: HDR-advertisement-filtered startup

- Date planned: 2026-07-19
- Outcome: **failed after confirmed filter activation**
- Rollback: **later performed solely for the Experiment 0005 clean rebuild**

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

This section records the original run plan. The user superseded the automatic
operational rollback assumption after the failure; see the dated run amendment
below.

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

## Run amendment: 2026-07-19 failure and post-mortem

### Installation and evidence

The user explicitly approved Experiment 0004. Source commit `e8ee4d2` was
rebuilt after the active loader, target fingerprint, pristine backup, restore
path, and preserved pipeline-cache state were checked. The build passed the
Bink re-export lookup, Rosetta self-patch transition `1 -> 2`, HDR-filter smoke
probe, eight startup-checker tests, and static checks. The bridge was installed
in `live-check` mode and the prepared evidence directory was created before the
user launch.

The user launched through the normal Steam and launcher path. ESO started at
2026-07-19 21:56:17 KST and crashed about 1.79 seconds later, before character
selection. Evidence collection selected run
`20260719T125618.172357000Z-pid85927`; all 14 files in its checksum manifest
passed verification. The files establishing the failure have these hashes:

| File | SHA-256 |
|---|---|
| `bridge-log-after.txt` | `a4615ba77e4e68eca2c38476622a16dea8914f8623bc5f1cc0e1ea31691340a3` |
| `startup-verdict.txt` | `260a5af76f8ef948f570e8d5218a67020c46c2d853a7093e6fceb4c79a1d09de` |
| `system-eso-2026-07-19-215622.ips` | `ae2f6a0aaeae040255433b627986ff85595d36ef4f077d6bd7bbfcd9df72cb8d` |

Raw evidence remains under the ignored
`artifacts/experiment-0004-20260719T124959Z/` directory and is not committed.

### Confirmed result

The bridge loaded MoltenVK 1.4.1 and redirected all 17 entries. Its device
extension wrapper observed 131 raw properties, exposed 130, and removed exactly
one `VK_EXT_hdr_metadata` property in both count and data calls. ESO then
created a device successfully with exactly these three extensions and did not
enable HDR metadata:

```text
VK_KHR_swapchain
VK_KHR_maintenance1
VK_EXT_debug_marker
```

Despite that transition, ESO queried `vkSetHdrMetadataEXT` through GDPA and
received NULL. The automatic verdict therefore failed exactly as designed.
The crash was main-thread `EXC_BAD_ACCESS / SIGSEGV` at address zero with
`RIP=0`, `RAX=0`, and the same first two ESO offsets (`0x364a7a5` and
`0x3608fcb`) as Experiment 0002. The loaded-image list contains the bridge,
pristine Bink companion, and MoltenVK 1.4.1.

The Rosetta state makes the immediate call site recoverable even though the
ordinary frame list starts at the outer virtual call. For both Experiments 0002
and 0004, `rosetta.tmp1` equals the ASLR-adjusted address
`image_base + 0x364c5c2`, the instruction immediately after ESO's indirect
`vkSetHdrMetadataEXT` call. This confirms that the NULL HDR setter was called;
it is no longer merely the last logged lookup.

### Root-cause refinement

Static disassembly shows that the setter path is guarded by an ESO object flag,
not directly by the device-extension list. ESO sets that flag when surface
format enumeration contains this exact pair:

```text
format     VK_FORMAT_A2B10G10R10_UNORM_PACK32 (64)
colorSpace VK_COLOR_SPACE_HDR10_ST2084_EXT     (1000104008)
```

An Apple M4 non-game surface probe returned three sRGB formats from embedded
MoltenVK 1.0.18 and no such pair. MoltenVK 1.4.1 returned 60 formats and did
include the pair. Hiding `VK_EXT_hdr_metadata` does not alter surface-format
enumeration, so Experiment 0004 left the actual ESO branch condition intact.

The Experiment 0004 hypothesis is therefore falsified: HDR device-extension
advertisement alone did not select the unsafe path. The confirmed incompatibility
is the combination of a newly visible HDR surface format, ESO's selection of
that format, a device created without `VK_EXT_hdr_metadata`, and ESO's
unchecked device-proc call.

### Rollback state and follow-up

After evidence collection, the user explicitly directed that rollback should
not be treated as an operational requirement. The installed Experiment 0004
state and marker remain in place as a failed checkpoint. Restoration is
allowed only when it becomes technically necessary for comparison, rebuilding,
or the next experiment; no claim is made that the installed state is usable.

The next source candidate filters only the exact surface-format pair above,
while retaining the Experiment 0004 extension filter and device tracing. Its
separate plan is [Experiment 0005](0005-hdr-surface-format-filter-startup.md).

## Second amendment: technical restore for Experiment 0005 rebuild

The failed installed state was not restored merely to produce a normal
operating state. It remained intact until Experiment 0005 source work reached a
hard build prerequisite: `scripts/build.sh` correctly refuses to use an active
proxy as its Bink input.

After the user reported that Steam and the launcher were stopped, process checks
found no ESO, launcher, or Steam process. The 14-file Experiment 0004 checksum
manifest was verified again before the transition. At approximately
2026-07-19 22:40 KST, `scripts/restore.sh` restored the pristine loader and old
pipeline cache. The displaced enable marker was retained as
`.teso4m4-enable.disabled-20260719-224048`; earlier displaced markers and the
raw crash evidence were also retained.

Post-restore status reported the known ESO fingerprint, original/inactive
loader, and absent enable marker. The active and pristine Bink files both had
SHA-256
`c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`.
This closes Experiment 0004's installed checkpoint only because the clean
rebuild required it; the failed result and its evidence remain unchanged.
