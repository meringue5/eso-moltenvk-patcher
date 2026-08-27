# Experiment 0041: inactive application pacing bypass

- Date: 2026-08-27
- Outcome: **candidate installed; first two recorded starts had normal FPS with pink and `active=yes`; natural recurrence gate remains**
- Rollback: **available and verified through the pristine loader; caches preserved**

## Question

Can the exact ESO 12.0.8 inactive-application outer-loop sleep be bypassed
without changing MoltenVK behavior, focus-event propagation, settings, caches,
or the normal launcher path, so a stale internal active-state byte cannot hold
the client near 10 FPS?

## Candidate

The new source-only `startup-inactive-pacing-bypass` mode retains the complete
0.1.2 `startup-pipeline-timing-control` configuration:

- official MoltenVK 1.4.2;
- live-resource checking disabled and Metal argument buffers disabled;
- MTLHeap and command pooling enabled;
- asynchronous queue submission and non-maximized pipeline compilation;
- bounded timing around the first 64 graphics-pipeline calls; and
- compositor neutralization and readiness canary disabled.

It adds one exact-target ESO patch. The target profile validates the complete
12-byte block beginning at image offset `0x30f78`:

```text
75 27 bf a0 86 01 00 e8 8a 78 8c 03
```

That block is the conditional skip, `mov edi, 100000`, and relative
`usleep` call. It is replaced in the running process by a same-size absolute
call to a bridge hook. The hook samples the internal application-active byte
at image offset `0x4a0c93c`, logs the first state and at most 15 subsequent
transitions, and returns immediately. Execution then rejoins ESO after the
removed sleep. Both active and inactive states therefore keep ESO's normal
outer loop; existing AppKit focus callbacks and downstream focus-event
propagation are not modified.

The inactive patch is prepared and committed in the same code-page transaction
as the 17 Vulkan redirects. Every destination and original byte sequence is
resolved before any page becomes writable. The added patch must stay within
one page, uses `VM_PROT_COPY`, clears the instruction cache, and participates
in the fatal RX-restoration invariant. An unknown executable hash, missing
target profile, semantic mismatch, byte mismatch, page-boundary error, or
permission error stops before the new write.

## Non-game evidence

All available agent-controlled gates pass:

```text
target: ESO 12.0.8 / databuild 3288357 / exact SHA-256 recognized
bridge build: PASS with -Wall -Wextra -Werror
Bink re-export: PASS
Rosetta self-patch: PASS
inactive pacing probe: PASS
  original mismatch rejected
  absolute call generated
  inactive state bypassed and logged
  active transition forwarded and bounded
  duplicate install rejected
  code page restored RX
MoltenVK candidate configuration: PASS
Python tests: 134 PASS
release installer transaction regression: PASS
Python compile / shell syntax / git diff check: PASS
MoltenVK 1.4.2 Metal compatibility probe: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probe: PASS on Apple M4
exact post-patch control-flow review: PASS
bridge architecture and original-Bink re-export: PASS
pristine restore identity against target manifest: PASS
```

The exact 12.0.8 disassembly rejoins at `0x30f84`, jumps to `0x30f9e`,
reloads the loop-running byte into `al`, and tests it immediately. It therefore
does not consume caller-saved registers or flags from the replaced branch.
The live loop state remains in `rbx`, and `rbx` plus `r12` through `r15` are
callee-saved by the hook's x86_64 ABI.

Built candidate identities:

```text
bridge SHA-256:   2b7520c28a514abc2a2991340f1e6c0be5ba582fc6dc39b3f568acc73dba40b8
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
original Bink:    f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
```

## Installation state

The exact target, installed 0.1.2 bridge, restore source, official runtime, and
all preserved cache identities passed before mutation. The first attempt
stopped safely because the shared bundle-idle gate found the ZeniMax launcher
running. After the user closed it, the gate reported idle Steam with no ESO
file or update activity.

The cache-preserving restore first produced the expected original state:
original loader active, enable marker absent, exact ESO target still selected,
and all three recorded pipeline-cache identities passing. The candidate was
then installed in `startup-inactive-pacing-bypass` mode. Post-install status
selected the same exact target, reported the bridge and MoltenVK current, and
reported every cache identity passing. Installed payload hashes matched the
built artifacts exactly:

```text
bridge:        2b7520c28a514abc2a2991340f1e6c0be5ba582fc6dc39b3f568acc73dba40b8
pristine Bink: c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb
original Bink: f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK:      aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
marker:        startup-inactive-pacing-bypass
```

The pristine, retagged-original, and replacement-runtime identities match the
selected target manifest rather than only matching local build copies. The
bridge is an x86_64 Mach-O and re-exports
`@loader_path/libBink2Macx64.teso4m4-original.dylib`; the replacement MoltenVK
contains both x86_64 and arm64 slices, including the x86_64 slice used by ESO
under Rosetta.

No game, launcher, or Steam process was launched by the agent.

## User-controlled validation gate

One ordinary Steam-authenticated launch is now the remaining first validation.
The unresolved questions are whether ESO starts and plays normally and whether
the bridge observes `active=no` while performance remains normal. Pink is
expected and is not a failure. Stop on a crash, persistent solid/black output,
or another severe rendering regression. The bridge state-transition record and
the user's FPS observation determine the result; no forced repetition is
required.

## Documentation promotion

The read-only reverse-engineering result is broader than this individual run.
Its durable component map, outer-loop control flow, activation callbacks,
shared state, renderer boundary, and update invariants are maintained in
[ESO host runtime structure](../ESO-HOST-RUNTIME.md). This experiment remains
the evidence record for the candidate design, installation, and user result.

## Initial user-controlled observations

The first two exact bridge starts after installation are:

```text
20260827T044900.700688000Z-pid17430
20260827T050537.435829000Z-pid19907
```

The user reported no low-FPS recurrence across these starts and continued to
observe the pink startup output. Both runs selected
`startup-inactive-pacing-bypass`, installed the exact host-loop patch, and
recorded only the initial state `active=yes`; neither recorded an `active=no`
transition. Graphics-pipeline call 5 arrived at 13.616 seconds in the first run
and 12.193 seconds in the second, then returned successfully in 1.568 and 1.420
milliseconds respectively. All 64 retained calls in both runs succeeded with
non-null output.

These starts pass the initial no-regression gate and provide a clean structural
separation: visible pink can coexist with an active host state and normal FPS.
They do not yet prove that bypassing the sleep repairs a naturally occurring
bad state, because neither process observed the internal byte as false. Do not
force launches to manufacture that state. The decisive future observation is
the first natural `active=no` or low-FPS start, classified against the user's
visible result.
