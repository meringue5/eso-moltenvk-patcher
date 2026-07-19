# Experiment 0002: live-check proc-traced startup

- Date: 2026-07-19
- Outcome: **failed after activation**
- Rollback: **complete; original loader and old pipeline cache restored**

## Question

Does a startup-only Steam launch with MoltenVK 1.4.1, live-resource checking,
and complete instance/device proc tracing reproduce Experiment 0001, and if so,
which Vulkan proc lookup or startup phase immediately precedes the fault?

## Hypothesis

The static and non-game probe results predict that all Vulkan functions ESO
queries will be non-null in MoltenVK 1.4.1. If the same `RIP=0` crash occurs
without a preceding null GIPA or GDPA result, an incomplete public Vulkan proc
table becomes less likely. A changed outcome only under live-resource checking
would support, but not prove, the descriptor-lifetime hypothesis.

## Target and change set

- ESO SHA-256: `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`
- ESO Mach-O UUID: `867e93bc-a6e7-3109-bf8e-542ff59ccdff`
- MoltenVK: official 1.4.1 release fetched through the pinned repository script
- Bridge source commit: `7a235dc`
- Redirect set: the same 17 validated entries used by Experiment 0001
- Marker mode: `live-check`
- Diagnostic change: trace both `vkGetInstanceProcAddr` and the
  `vkGetDeviceProcAddr` returned through it, including handle, name, address,
  and explicit null markers
- Safety change: any failure to restore a patched code page to RX exits the
  process instead of continuing

This is a startup evidence run, not a gameplay or performance test.

## Preflight

- `scripts/status.sh` confirmed the target hash, original Bink loader, and
  absent enable marker after Experiment 0001 rollback.
- Mach-O analysis recovered 162 old Vulkan text symbols and found 40 external
  references to 17 entry points: 39 direct calls/jumps to 16 wrappers plus one
  address-taken `vkGetInstanceProcAddr`. The manifest covers all 17.
- The new runtime exports all 17 externally referenced entries. Three removed
  private MoltenVK functions have no recovered external reference.
- Static query analysis recovered every direct lookup site: 19 GIPA calls to 17
  unique names and 80 GDPA calls to 65 unique names.
- The old static 1.0.18 and new dynamic 1.4.1 runtimes were queried with the
  same 100-name candidate set after creating an instance and device. On the
  route ESO actually uses for each name, no old-nonnull/new-null result was
  found.
- Both runtimes created an instance and device on the Apple M4 in the non-game
  probe. The old runtime reported two requested device extensions as
  unsupported but tolerated them; the new runtime reported them supported.
- A clean source build passed Bink re-export lookup and the Rosetta self-patch
  transition from 1 to 2.
- Evidence preparation and collection scripts passed a no-launch smoke run.

These checks establish public wrapper and proc-lookup coverage for this exact
build. They do not prove compatible private ABI, callbacks, object lifetime,
surface/swapchain behavior, or gameplay stability.

Post-run amendment: the initial proc comparison created the new device without
enabling the new runtime's advertised `VK_EXT_hdr_metadata` extension. It
therefore verified a single extension-negotiation state, not all states that
ESO could select after seeing a larger advertised extension set.

## Procedure

1. Obtain explicit approval for this exact game-bundle modification.
2. Re-run `scripts/status.sh`, confirm ESO is stopped, and verify the restore
   path and pristine Bink backup.
3. Install commit `7a235dc` in `live-check` mode using the experimental gate.
4. Run `scripts/prepare-evidence.sh` and retain its output directory.
5. The user launches ESO through Steam. Do not bypass Steam authentication.
6. Stop the test at the first of: a crash, character selection becoming stable,
   or 60 seconds after the ESO process starts. Do not enter the game world.
7. Run `scripts/collect-evidence.sh` on the prepared directory.
8. Run `scripts/restore.sh` immediately and verify the original loader and old
   pipeline cache with `scripts/status.sh`.

Agents must not launch ESO, Steam, or the launcher.

## Evidence

Raw evidence is preserved under ignored directory
`artifacts/experiment-0002-20260719T060809Z/`. It contains the before/after
status snapshots, bridge log, one new `.ips`, unified log, timestamps, and a
checksum manifest. The files that establish the failure have these SHA-256
hashes:

| File | SHA-256 |
|---|---|
| `bridge-log-after.txt` | `1a17a7cbb6405fdb3caf9ea34a6551c6513adc6560a07443a15c5f172f2f8cfc` |
| `system-eso-2026-07-19-151031.ips` | `f862334e77b518f67cb49c634463cc0a63e6aa3495059121b1c6c9beacf6324c` |
| `eso-unified.log` | `f7132bf6fc00031bcdab7e47c6c7b93c55cf27deb19dc108b3e2c4323f8c2100` |

`shasum -c SHA256SUMS` passed after collection. Raw evidence is intentionally
not committed because it may contain local paths or identifiers.

## Result

The user explicitly approved this installation. At 2026-07-19 15:08 KST, the
bridge was installed in `live-check` mode after creating and byte-verifying the
previously absent pristine Bink backup. The old pipeline cache was moved to its
preserved backup path, the enable marker was verified, and evidence collection
was prepared.

The user then launched through Steam. MoltenVK 1.4.1 activated and all 17
manifest entries were redirected. Startup crashed at 2026-07-19 15:10:31 KST.
The bridge log ended with these successful device queries followed by one null
result:

```text
GDPA: ... vkCreateSwapchainKHR -> non-null
GDPA: ... vkCreateSemaphore -> non-null
GDPA: ... vkSetHdrMetadataEXT -> 0x0 [NULL]
```

The crash was `EXC_BAD_ACCESS / SIGSEGV` on the main thread with `RIP=0`,
`RAX=0`, and `KERN_INVALID_ADDRESS at 0`. The first recoverable ESO return
address was again executable offset `0x364a7a5`, exactly matching Experiment
0001. The loaded-image list included the bridge, pristine re-export library,
and MoltenVK 1.4.1. The collected unified log did not add a relevant Vulkan or
MoltenVK message before the crash.

A post-run non-game probe established the extension-dependent behavior:

| Runtime/device state | Advertises `VK_EXT_hdr_metadata` | GIPA | GDPA |
|---|---:|---:|---:|
| MoltenVK 1.0.18 | no | NULL | NULL |
| MoltenVK 1.4.1, extension not enabled | yes | non-null | NULL |
| MoltenVK 1.4.1, extension enabled | yes | non-null | non-null |

## Interpretation

Confirmed observations are that ESO reached device and swapchain setup, the
last recorded proc lookup returned NULL for `vkSetHdrMetadataEXT`, and the
subsequent crash reproduced Experiment 0001's null instruction pointer and ESO
return offset. The new runtime exposes a conditional HDR path that the old
runtime did not advertise.

The strongest current inference is an extension-negotiation mismatch: ESO may
select an HDR-metadata path because MoltenVK 1.4.1 advertises the extension,
then create a device without enabling that extension and eventually call the
null device proc. The trace does not prove that the last returned pointer was
the one called, nor does it record the enabled-extension list passed to
`vkCreateDevice`, so this is not yet a confirmed root cause.

The original hypothesis that the 100-name probe ruled out a public-proc
availability problem was too broad. Proc availability depends on which device
extensions were enabled, and the first probe did not model that conditional
state.

## Rollback

The bridge was restored immediately after collection. At 2026-07-19 15:15 KST,
`scripts/status.sh` reported the active Bink loader as original/inactive and the
enable marker as absent. The active loader byte-matches the pristine backup,
and the old pipeline cache was restored. The former enable marker was preserved
under a disabled timestamped name rather than deleted.

## Follow-up

Before another installation, wrap or intercept `vkCreateDevice` in a non-game
diagnostic path and record its enabled device-extension names. Then reproduce
the exact advertised-versus-enabled state and choose between two narrowly
scoped compatibility experiments: hide `VK_EXT_hdr_metadata` during device
extension enumeration to emulate 1.0.18, or supply a guarded compatibility
implementation for the null HDR setter. Filtering the advertised extension is
the more principled first candidate, but neither approach is yet validated.
