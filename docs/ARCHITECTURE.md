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
- maximum concurrent compilation enabled.

After `dlopen`, the bridge reads the effective `MVKConfiguration` and stops
before patching if any required value differs. The settings are one validated
profile, not independently supported toggles.

## Bounded startup compositor neutralizer

The final scene-and-GUI compositor can present ESO's canonical-magenta
placeholder during a proven early startup interval. On the exact supported
client, the bridge tracks the verified generation, pipeline, pipeline layout,
descriptor-set layouts, descriptor classes, and framebuffer identity.

For generation 2, presentation ordinals 71 through 149, an exact matching
compositor draw is replaced by an opaque-black full-frame clear. At ordinal
150 the normal application draw is forwarded and the bridge permanently
latches to forwarding. Unexpected state, incomplete provenance, capacity
limits, identity mismatch, or a missing clear destination all fail open to ESO's
original draw. The production path performs no pixel readback or queue-idle
synchronization.

This is a bounded presentation repair. It does not modify ESO assets, shaders,
settings, or the underlying placeholder input. The evidence chain is preserved
in Experiments 0026 through 0031.

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
update recovers whether the launcher restored the original loader or retained
the stale bridge loader. Interrupted transactions are restored to the verified
baseline before restarting.
Uninstall restores the verified original and preserves later user settings
when they differ from the installer's recorded merge result.

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
instrumentation shared by the production neutralizer and future exact-target
diagnosis. These facilities are not public configuration promises. Abandoned
1.4.1 source patches and failed legacy feature masking are not part of the
production tree.

See [Production baseline](PRODUCTION.md), [Project status](STATUS.md), and
[Logging policy](LOGGING.md) for the supported scope and operational behavior.
