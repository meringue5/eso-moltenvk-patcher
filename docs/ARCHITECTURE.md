# Bridge architecture

This document describes the current MoltenVK 1.4.2 production path. Historical
1.4.1 hypotheses, failed modes, and diagnostic procedures remain in
[experiments](experiments/README.md) and [research](research/README.md); they are
not supported runtime configurations.

## Runtime path

ESO dynamically loads `@executable_path/libBink2Macx64.dylib`, but its MoltenVK
1.0.18 code is statically linked into the game executable. The production
package uses the Bink load point for a small x86_64 proxy that:

1. re-exports the complete original Bink interface from a verified renamed
   pristine library;
2. runs a constructor during ESO startup;
3. verifies the executable SHA-256 attested by the installer and every original
   patch-site byte; legacy one-line markers additionally require the compiled
   exact SHA-256 and Mach-O UUID;
4. loads the pinned official MoltenVK 1.4.2 dynamic library;
5. resolves every required destination before changing memory;
6. redirects 17 verified externally referenced Vulkan entry points; and
7. restores patched code pages to RX permissions on success and error paths.

The ESO executable is never rewritten on disk. Code redirection exists only in
the running process. Mach-O text pages are patched through a private
`VM_PROT_COPY` mapping, followed by instruction-cache invalidation and RX
restoration.

## ESO host loop outside MoltenVK

ESO's AppKit host loop is a distinct control layer above the Vulkan bridge.
Read-only analysis of the exact 12.0.8 executable shows that its outer loop
sends `platformMainLoop`, reads a process-global application-active byte, and
calls `usleep(100000)` when that byte is false. The activation and resignation
callbacks feed true and false through one setter to the same byte. This fixed
inactive path can impose an approximately-10-Hz cadence before graphics-
pipeline creation and is separate from both MoltenVK and ESO's configurable
background-FPS limit.

The installed Experiment 0041 source candidate is not yet a production release
profile. It retains the 0.1.2 MoltenVK path and replaces only the exact inactive
sleep block with a bounded state-observing hook that returns without sleeping.
It does not alter AppKit callbacks or synthesize focus. The exact build map,
control-flow offsets, confidence boundaries, renderer relationship, and update
invariants are maintained in
[ESO host runtime structure](ESO-HOST-RUNTIME.md); the run-specific evidence
remains in [Experiment 0041](experiments/0041-inactive-pacing-bypass.md).

## Compatibility layer

ESO was written against embedded MoltenVK 1.0.18 behavior. The production
bridge retains two narrow compatibility rules while otherwise forwarding calls
to official MoltenVK 1.4.2:

- remove `VK_EXT_hdr_metadata` from device-extension enumeration; and
- remove only the HDR10
  `VK_FORMAT_A2B10G10R10_UNORM_PACK32` /
  `VK_COLOR_SPACE_HDR10_ST2084_EXT` surface-format pair.

The filters preserve Vulkan count/data semantics, ordering, layer passthrough,
and `VK_INCOMPLETE` behavior. Device creation is forwarded with ESO's original
arguments after logging and validating the effective runtime state.

## Production MoltenVK configuration

Before loading MoltenVK, the bridge selects the validated production settings:

- live-resource checking disabled;
- Metal argument buffers disabled;
- MTLHeap enabled where safe;
- asynchronous queue submission;
- command pooling enabled;
- no command-buffer prefill; and
- maximum concurrent compilation disabled.

After `dlopen`, the bridge reads the effective `MVKConfiguration` and stops
before patching if any required value differs. The settings are one validated
profile, not independently supported toggles.

## Stable bounded startup control

The 0.1.3 production profile combines two exact-target repairs. It replaces
ESO's fixed inactive `usleep(100000)` branch with an observe-and-return hook
without patching the AppKit callbacks or active-state byte, and it suppresses
only the proven final-compositor placeholder draws at generation-2 ordinals 71
through 149.
The compositor path latches permanently to direct forwarding at ordinal 150
and the bounded startup lifecycle finishes at ordinal 180.

Graphics-pipeline timing, pixel readback, readiness canaries, and compositor
image sampling are disabled. The startup identity wrappers required for the
fail-open repair remain active only through the bounded window; every retained
destroy/free/reset wrapper checks the common finished gate and direct-forwards
without lifecycle-table mutation afterward.

Experiments 0038 and earlier timing/audit modes remain diagnostic history, not
the 0.1.3 production path. Experiments 0044 and 0045 establish the functional
repair and its measurement-stripped release form.

## Installer transaction

The release installer operates on the selected `eso.app`, not on a Steam-only
path. It searches known Steam and ZeniMax locations and accepts a player-chosen
bundle when automatic discovery fails.

Before mutation it verifies:

- either the exact selected executable or a relocation-tolerant native audit
  of unchanged embedded MoltenVK, patch bytes, text-reference boundary, and
  proc-query shape;
- original loader identity or an exact known inactive development state;
- payload hashes and companion files;
- absence of ESO, launcher, active update, and bundle file holders; and
- an available verified restore record.

Install writes a journaled per-installation state, preserves the original
loader, stages files before atomic replacement, records the package version,
attests the audited executable SHA-256 in the enable marker, and verifies the
resulting bridge/runtime/marker identity. Re-running Install after a launcher
update reuses a launcher-restored original only when it belongs to the release's
supported generation. A retained bridge is not evidence of the launcher's
current original, so Install and Uninstall stop and require launcher Repair
instead of restoring an older backup. When a newer release explicitly supports
a different original generation, it archives the prior state, backup, and
marker before making a fresh backup. Interrupted transactions are restored to
their verified same-generation baseline before restarting.

Uninstall applies the same generation rule. It restores a backup only when the
active bridge, marker attestation, executable, and recorded backup belong to the
same generation. If the launcher already supplied a non-bridge loader,
Uninstall leaves that active file byte-for-byte unchanged and removes only
exact patch-owned companions. Later user settings remain preserved when they
differ from the installer's recorded merge result.

Historical 1.4.1 runtime and cache backups are recognized only so maintenance
and restore operations do not destroy evidence. They are not packaged payloads
or selectable production runtimes.

## Update gate

`config/current-target.txt` selects the supported manifest. The source update
checker compares executable SHA-256, Mach-O UUID, client version, databuild,
patch-site bytes, embedded archive members, external-reference shape,
proc-query routes, and official replacement-runtime identity. Reference and
query source addresses may relocate; symbol, kind, count, route, and recovered
name semantics must remain equal.

The release package also contains a native compatibility fingerprint compiled
from the selected baseline. A non-exact executable may be installed only when
its embedded archive hash matches and the auditor reproduces all compiled
patch signatures, old-runtime boundary references, and proc-query
multiplicities. The resulting executable hash is recorded for the runtime to
enforce. Any mismatch stops for manual analysis. Source rebase, build, release
assembly, install, and user-controlled launch validation remain separate
maintenance gates.

## Diagnostics and historical code

The source retains bounded lifecycle, descriptor, draw, and compositor
instrumentation for historical experiments and future exact-target diagnosis.
The 0.1.3 release retains only bounded operational evidence for the startup
repair and disables pipeline timing and post-window bookkeeping. These
facilities are not public configuration promises. Abandoned
1.4.1 source patches and failed legacy feature masking are not part of the
production tree.

See [Production baseline](PRODUCTION.md), [Project status](STATUS.md), and
[Logging policy](LOGGING.md) for the supported scope and operational behavior.
