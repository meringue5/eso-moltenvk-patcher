# AGENTS.md

This file is the operational memory for coding agents working on `teso4m4`.
Read it before changing code, running a game test, or drawing conclusions from
older experiments.

## Mission

`teso4m4` investigates Steam ESO behavior and performance on Apple Silicon,
with a current focus on replacing or bridging ESO's statically linked MoltenVK
1.0.18 runtime without breaking Steam launch and authentication.

This is a research project, not a working gameplay mod. Accuracy, reversibility,
and evidence preservation take priority over obtaining a quick FPS result.

## Start every task here

1. Run `git status --short --branch` and do not overwrite unrelated work.
2. Read these files in order:
   - `README.md`
   - `docs/CRASH_ANALYSIS.md`
   - `docs/experiments/0001-moltenvk-1.4.1-full-redirect.md`
   - `docs/ROADMAP.md`
   - `docs/ARCHITECTURE.md`
3. Confirm that the active game loader is original before assuming a clean
   baseline. `scripts/status.sh` is the non-destructive status check.
4. Rebuild from source after bridge changes. Never rely on a stale `build/`
   directory.

## Established facts

- The analyzed Steam macOS ESO executable is x86_64 and runs under Rosetta.
- Analyzed executable SHA-256:
  `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`
- Mach-O UUID: `867e93bc-a6e7-3109-bf8e-542ff59ccdff`.
- Bundled headers identify MoltenVK 1.0.18.
- ESO statically links MoltenVK. The bundled framework payload is a static `ar`
  archive, and ESO has no dynamic Vulkan/MoltenVK dependency.
- Replacing the bundled MoltenVK archive alone cannot change the linked runtime.
- Official MoltenVK 1.4.1 successfully created a Vulkan 1.0 instance and device
  on Apple M4 with ESO's observed legacy extension set.
- A Bink re-export proxy can load before ESO and can patch x86_64 text entries
  under Rosetta using `mach_vm_protect(..., VM_PROT_COPY)`.
- Direct-call analysis found 39 ESO calls/jumps to 16 static Vulkan wrappers.

## Experiment 0001 result

The first bridge redirected `vkGetInstanceProcAddr` plus those 16 directly
referenced Vulkan wrappers to MoltenVK 1.4.1. The bridge log confirmed all 17
redirects were active.

ESO then crashed approximately 3.1 seconds after launch:

- `EXC_BAD_ACCESS / SIGSEGV`
- main thread
- `RIP=0`
- early Vulkan/Metal initialization
- first recoverable ESO return address near unslid `0x10364a7a5`
- last ESO unified-log event was a Metal compiler warning at the same millisecond

The loaded-image list confirmed MoltenVK 1.4.1 was active. The null instruction
pointer makes a missing/unpopulated function or callback the leading hypothesis.
Old/new MoltenVK handle mixing remains another serious possibility.

Do not repeat Experiment 0001 unchanged.

## Current baseline outside the repository

After Experiment 0001, the active Bink loader and old pipeline cache were
restored. The pristine and active Bink SHA-256 values matched at the last check.
Experimental companion dylibs may remain beside the game executable, but they
are inactive when the pristine Bink loader is active.

Always verify this state rather than assuming it persisted across Steam updates
or later user activity.

## Immediate technical objective

Explain the startup null call before attempting performance measurement.

The current source wraps `vkGetInstanceProcAddr` and logs every requested name,
returned address, and null result. The next controlled launch should use this
trace and `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1`, then stop at startup evidence
collection. It is not a gameplay test.

Before that launch:

1. Make Vulkan wrapper cross-reference analysis exhaustive. Direct `E8`/`E9`
   scanning alone does not cover address-taken functions, tables, or relocations.
2. Review all functions queried through `vkGetInstanceProcAddr` and compare old
   1.0.18 behavior against 1.4.1.
3. Ensure every path receiving new MoltenVK handles stays in new MoltenVK.
4. Prepare crash, bridge-log, and unified-log collection before installation.

## Safety rules

- Never launch ESO, Steam, or the launcher on the user's behalf. The user owns
  login and interactive game control.
- Never install the bridge or modify the game bundle without explicit approval.
- Never bypass the Steam authentication path or launch `eso.app` directly as a
  substitute for a Steam test.
- Never delete the pristine Bink backup, old pipeline cache, crash evidence, or
  user settings. Preserve and rename before replacement.
- Never use destructive Git commands or revert user changes.
- Unknown ESO hashes and UUIDs must fail closed.
- Validate every original patch-site byte before writing any patch.
- Resolve every destination before making any code page writable.
- Restore RX page permissions after patching, including error paths.
- A restore path must be available and checked before every install test.
- Treat a Steam update as an unknown build until fingerprints and offsets are
  re-established.

## Repository hygiene

Do not commit:

- ESO executables or app bundles
- Bink, Steam, ESO SDK, or extracted proprietary object files
- MoltenVK release binaries or archives
- pipeline/shader caches
- full `UserSettings.txt`
- raw `.ips` reports
- account names, email addresses, machine-specific absolute paths, or tokens

Small build fingerprints, offsets, byte signatures, sanitized settings excerpts,
and sanitized crash facts are allowed when needed for reproducibility.

`build/`, `vendor/`, logs, caches, dylibs, archives, and object files must remain
ignored. Fetch MoltenVK from the official pinned release through
`scripts/fetch-moltenvk.sh`.

## Build and validation

Normal local sequence:

```sh
./scripts/fetch-moltenvk.sh
./scripts/build.sh
python3 -m compileall -q tools
zsh -n scripts/*.sh
git diff --check
```

Expected non-game smoke checks from `scripts/build.sh`:

- Bink symbol re-export lookup succeeds.
- Rosetta self-patch probe changes its test result from 1 to 2.

The Vulkan compatibility probe may require execution outside a restricted
sandbox to access Metal. Running that probe is not permission to launch ESO.

## Installation gate

The installer is intentionally blocked unless the caller sets:

```sh
TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK
```

That environment variable is not standing user approval. An agent must still
receive explicit approval for each modification of the game bundle.

The current default experimental marker mode is `live-check`; it is unproven.
Do not describe it as a fix.

## Evidence standards

- Separate confirmed observation, inference, and hypothesis in documentation.
- Record exact build hashes, runtime versions, timestamps, and test modes.
- For performance A/B tests, hold zone, camera, resolution, settings, player
  density, and test duration as constant as possible.
- Capture FPS, GPU time, frame interval, app memory, Metal memory, and thermal
  state. FPS alone is insufficient.
- Do not claim a resource leak merely from increasing memory. The current
  evidence supports accumulation or retained state, not a proven leak source.
- The Metal HUD render-pass warning describes engine/render-graph behavior. Do
  not promise that a MoltenVK swap can automatically merge ESO render passes.

## Continuity protocol

At the end of substantial work:

1. Update the relevant experiment document with result, evidence, and rollback
   state, including failed experiments.
2. Update `docs/ROADMAP.md` when the leading hypothesis or next gate changes.
3. Update this file only when durable operational facts or safety rules change.
4. Run static checks and confirm `git status` is understood.
5. Commit a coherent unit of work with a message that names the experiment or
   analysis performed.

Conversation history is supporting context. The repository documents and Git
history are the authoritative source of project state.
