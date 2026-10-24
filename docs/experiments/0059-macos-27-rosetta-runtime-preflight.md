# Experiment 0059: macOS 27 Rosetta and runtime preflight

- Date: 2026-10-24
- Outcome: **succeeded in non-game preflight; ESO runtime acceptance untested**
- Rollback: **not required; installed files unchanged**

## Question and hypothesis

Does the current macOS environment still execute the x86_64 code and memory
patch mechanism required by the bridge, and can pinned MoltenVK 1.4.2 still
create an ESO-era Vulkan device? Failure of translation, the self-patch probe,
configuration query, or device creation would falsify the local hypothesis.

## Target and scope

- Source: `86ca8ed`; no bridge or probe source changed.
- macOS 27.0.1, build `26A434`, arm64 host / Apple M4.
- ESO 12.1.6 / databuild 3309674, executable SHA-256
  `3bdcfda575da791d6a337ef34afe3390d0a373aacdcaad033b2b454ddce804e6`.
- Official MoltenVK 1.4.2, dylib SHA-256
  `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
- Observation checkpoint: `2026-10-24T01:58:35Z` (10:58:35 KST).

## Procedure and evidence

1. Recheck update/status before interpreting the target. `UNKNOWN_UPDATE` and
   stale bridge attestation remain; the active Bink path is still a bridge,
   not an original-loader baseline.
2. Read OS version/build, Rosetta receipt, and payload architectures.
   `arch -x86_64 /usr/bin/uname -m` returns `x86_64`. ESO and the bridge are
   x86_64; pinned and installed MoltenVK contain x86_64 and arm64.
3. Build `tools/probe_self_patch.c` freshly into temporary storage with
   x86_64, deployment target 11.0, warnings-as-errors, and O0. It exits 0:
   `Rosetta entry patch: before=1 after=2`. Only the probe's own process is
   patched; it does not load the bridge or change game code.
4. Build `tools/probe_mvk_config.c` freshly with pinned headers. With
   `startup-compositor-neutralize-pacing-release`, it reports result 0,
   configuration size 184, live checking 0, argument buffers 0, MTLHeap 1,
   asynchronous submits, command pooling 1, prefill 0, maximum compilation 0,
   and `PASS`.
5. Build `tools/probe_vulkan.c` plus `src/mvk_compat.c` freshly. Run with the
   same production environment settings and `TESO4M4_PROBE_HDR_FILTER=1`.
   The restricted run cannot access Metal and returns
   `VK_ERROR_INCOMPATIBLE_DRIVER`; this is not classified as OS incompatibility.
   The approved non-game rerun outside the sandbox exits 0 on Apple M4:
   ESO-era instance/device creation succeeds and HDR filter validation passes.
   Proc summary: 100 candidates, GIPA-null 0, GDPA-null 17. These raw counts
   are not a claim that every query is valid on both routes.

A separate `sysctl.proc_translated` shell query was permission-denied in the
sandbox. It was not used as absence evidence. Successful x86_64 execution and
the fresh self-patch probe establish the narrower operational observations.

No ESO, Steam, or launcher was started. No graphical surface/game rendering,
bridge installation, cache reset, or setting change was performed. The probes
do not load the installed bridge or invoke its logging constructor. No stale
`build/` artifact was used.

## Result and limits

Local non-game prerequisites still work on this macOS 27.0.1 machine. This does
not prove the complete bridge works on the updated ESO target: Experiment
0058's original-loader provenance and installation gates remain unresolved.
It also does not test focus, FPS, rendering, or macOS 28.

The self-patch probe verifies its return-value transition; it is not a complete
audit of production memory-permission error paths. No runtime source change or
full production build is implied by compiling these three probes.

Installed executable, loader, runtime, production log, and vendor runtime were
inventoried before/after probes. All five retain identical SHA-256, size,
mtime, and inode. Settings/caches were not targeted; this is not a fresh
whole-installation or filesystem preservation audit.

The separate [upstream review](../research/macos-27-rosetta-moltenvk-review.md)
owns policy/release findings and primary sources. The design now requires
explicit platform capability reporting and retains MoltenVK 1.4.2.
Rollback is unnecessary; only source documentation changes are committed.

## Documentation checks

Python 3.14.8 bytecode compilation, Zsh script syntax, `git diff --check`, and
all 94 relative Markdown links in the changed documents pass. No source code
changed; the full Python suite was not repeated after Experiment 0058's
147-test pass. The three newly compiled probes above provide this run's local
capability evidence.
