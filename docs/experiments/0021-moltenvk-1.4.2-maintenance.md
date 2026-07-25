# Experiment 0021: official MoltenVK 1.4.2 maintenance adoption

- Date: 2026-07-26
- Outcome: **prepared; installation explicitly approved**
- Rollback: **Experiment 0020 remains installed; 1.4.1 runtime and cache transition are verified in a shadow install**

## Question

Can the official MoltenVK 1.4.2 macOS runtime replace 1.4.1 while retaining
Experiment 0020's exact `performance-aggressive` profile, avoiding a material
descriptor-path regression, and keeping both the 1.4.1 and pre-1.4.1 cache
states recoverable?

This is a maintenance adoption, not a claim that 1.4.2 repairs ESO's known
solid-output graphics reset.

## Candidate identity

The candidate is the official Khronos MoltenVK v1.4.2 macOS release:

```text
tag commit:     db66022459ffb663aa2b50f6b018bc2e124f5edf
archive SHA-256:
  f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e
universal dylib SHA-256:
  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
pipeline-cache UUID:
  db6602241a0502090000000100000000
```

The repository fetcher pins both the archive and runtime hashes. The selected
ESO target profile also pins the replacement-runtime hash, and both build and
install fail closed if it differs.

The complete review, upstream-change relevance, and benchmark limits are in
[the MoltenVK 1.4.2 adoption review](../research/moltenvk-1.4.2-adoption-review.md).

## Runtime profile

The bridge behavior is unchanged from successful Experiment 0020:

```text
MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=0
MVK_CONFIG_USE_MTLHEAP=1
MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0
MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1
MVK_CONFIG_USE_COMMAND_POOLING=1
MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0
```

No shader compression setting is added. Keeping the runtime upgrade separate
preserves attribution and does not consume another user test.

## Cache transition

MoltenVK 1.4.1 and 1.4.2 report different Vulkan pipeline-cache UUIDs. The
installer therefore implements one explicit
`mvk-1.4.1-to-1.4.2` transition:

1. require the current installed runtime to match the exact official 1.4.1
   SHA-256;
2. parse the 32-byte Vulkan header of the active cache and require exact
   1.4.1 UUID `db445ff21a0502090000000100000000`;
3. refuse to overwrite any existing versioned backup;
4. preserve the 1.4.1 runtime under
   `libMoltenVK.teso4m4-v1.4.1-backup.dylib`;
5. preserve the active cache under
   `PipelineCache.esopc.teso4m4-mvk-1.4.1-backup`;
6. leave `PipelineCache.esopc` absent so 1.4.2 creates its own cold cache;
7. leave `PipelineCache.esopc.teso4m4-old-backup` untouched.

Malformed, truncated, unknown-version, or wrong-UUID cache headers fail closed.
The restore path accepts either an active new cache or the preserved versioned
1.4.1 cache without moving or deleting either.

## Non-game validation

The candidate passed:

- official archive and universal-runtime SHA-256 verification;
- a fresh shadow-bundle source build using the current pristine ESO loader;
- Bink re-export and Rosetta self-patch probes;
- all twelve MoltenVK configuration modes, including exact
  `performance-aggressive`;
- all 87 Python tests, Python byte compilation, shell syntax, whitespace
  checks, warnings-as-errors compilation, and Clang static analysis;
- the real-Metal ESO-era device/proc profile and 1.4.1/1.4.2 core-profile
  comparison;
- both HDR compatibility filters;
- 24 alternating-resolution reset cycles on both 1.4.1 and 1.4.2 with the
  exact asynchronous aggressive profile, full-lifetime descriptor reuse, and
  alternating command-buffer/pool resets;
- a complete shadow install, cache transition, post-install hash comparison,
  and cache-preserving restore.

Prepared artifact SHA-256 values are:

```text
proxy:
  5019d4eb552f89ea59bfda9d38e2f2c98ce36f2490c7c41294750c62ba68acde
renamed original:
  f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK 1.4.2:
  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

A balanced descriptor-heavy comparison measured 1.4.2 2.293% slower than
1.4.1, below the existing 3% meaningful-change threshold. This is not an ESO
FPS measurement, but it finds no material regression in the path optimized by
Experiment 0020.

No Steam, launcher, or ESO process was started by the agent.

## Installation gate

The user explicitly approved adopting MoltenVK 1.4.2 and directed that
Experiment 0020 be recorded as successful. Before installation:

- re-run the update and status checks;
- require Steam, the launcher, and ESO to be stopped;
- verify the current settings, both existing caches, pristine loader, active
  1.4.1 runtime, and all prepared artifact hashes;
- restore Experiment 0020 with every cache state preserved;
- rebuild once more from the actual pristine game loader;
- run the approved cache transition and verify every post-install hash.

No additional user graphics-reset test is requested as part of this
maintenance adoption. Ordinary gameplay can validate the maintenance change.
