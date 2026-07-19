# Experiment 0002: live-check proc-traced startup

- Date: 2026-07-19
- Outcome: **running; installed, awaiting user-controlled startup**
- Rollback: **pending; pristine backup and old pipeline cache verified**

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

Planned evidence is the bridge log with GIPA/GDPA results, any new `.ips`, ESO
unified log since the prepared timestamp, target fingerprints, and before/after
status snapshots. Raw evidence stays under ignored `artifacts/` because it may
contain local paths or identifiers. Only sanitized facts and excerpts may be
added here.

## Result

The user explicitly approved this installation. At 2026-07-19 15:08 KST, the
bridge was installed in `live-check` mode after creating and byte-verifying the
previously absent pristine Bink backup. The old pipeline cache was moved to its
preserved backup path, the enable marker was verified, and evidence collection
was prepared. ESO, Steam, and the launcher remained stopped; the startup test
has not yet begun.

## Interpretation

No runtime interpretation is available before the controlled startup.

## Rollback

Pending after the user-controlled startup. The pristine Bink backup is present
and classified as original, the proxy re-export target is present, and the old
pipeline cache backup is present.

## Follow-up

Classify the outcome by the last GIPA/GDPA result and crash address. If no null
proc result precedes the same fault, investigate the ESO graphics abstraction
vtable, initialization callbacks, and surface/swapchain path before adding
broader Vulkan-call tracing.
