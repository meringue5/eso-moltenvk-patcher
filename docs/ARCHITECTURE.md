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
- maximum concurrent compilation disabled.

After `dlopen`, the bridge reads the effective `MVKConfiguration` and stops
before patching if any required value differs. The settings are one validated
profile, not independently supported toggles.

## Performance-first startup control

The 0.1.2 production profile deliberately does not replace ESO's early
canonical-magenta compositor draws. The original pink placeholder can remain
visible. This is an accepted cosmetic issue because the first no-neutralizer
control returned to normal FPS and the normal renderer-completion path, while
the preceding neutralized run reproduced the intermittent low-FPS path.

Only the first 64 `vkCreateGraphicsPipelines` calls receive a bounded timing
wrapper. Every argument and result is forwarded unchanged; all other lifecycle,
draw, descriptor, presentation, and compositor entry points are returned
directly to MoltenVK. The timing evidence distinguishes delayed ESO requests
from slow or failed downstream compilation without changing caches or adding a
readiness wait.

The former bounded neutralizer and its evidence remain preserved in
Experiments 0026 through 0031 and 0038. They are diagnostic history, not the
0.1.2 production path.

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
instrumentation for historical experiments and future exact-target diagnosis.
Only bounded pipeline timing is active in 0.1.2. These facilities are not
public configuration promises. Abandoned
1.4.1 source patches and failed legacy feature masking are not part of the
production tree.

See [Production baseline](PRODUCTION.md), [Project status](STATUS.md), and
[Logging policy](LOGGING.md) for the supported scope and operational behavior.
