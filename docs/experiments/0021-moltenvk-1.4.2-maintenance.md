# Experiment 0021: official MoltenVK 1.4.2 maintenance adoption

- Date: 2026-07-26
- Outcome: **succeeded in ordinary high-settings gameplay; live resets render correctly**
- Rollback: **1.4.1 runtime, its active cache, pristine loader, and older cache remain preserved**

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
source:
  79bb444 (Prepare Experiment 0021 MoltenVK 1.4.2)
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

## Installation

At 2026-07-26 02:05 KST, Steam, the ZeniMax launcher, and ESO were all stopped.
The current target gate identified exact ESO 12.0.7 SHA-256
`82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`.
Pre-install evidence verified Experiment 0020's loader, exact official 1.4.1
runtime, settings, active cache, and older cache.

Experiment 0020 was restored with every cache state preserved. The bridge was
then rebuilt from the actual pristine game loader at source commit `79bb444`;
the complete build passed and reproduced the shadow-build hashes. The approved
transition installed official 1.4.2 in unchanged `performance-aggressive`
mode.

Post-install verification reports:

```text
mode marker:
  performance-aggressive
proxy:
  5019d4eb552f89ea59bfda9d38e2f2c98ce36f2490c7c41294750c62ba68acde
renamed original:
  f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK 1.4.2:
  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
preserved MoltenVK 1.4.1:
  d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

The installed proxy, renamed original, and MoltenVK 1.4.2 are byte-identical
to the source rebuild. Cache and settings state is:

```text
active 1.4.2 cache:
  absent (intentional cold start)
preserved 1.4.1 cache:
  4,259,071 bytes
  5869aa929521788681e665e803e5876487a4eb8cede9589f56c48e05087c404b
  UUID db445ff21a0502090000000100000000
older cache:
  6,800,792 bytes
  72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
settings:
  2e69c076b1d9f10185175f2a6b0f5f3e14608552ad773e3594008f0de9215ed4
```

The settings and older-cache hashes did not change. The target gate and
post-install process gate passed, and the active runtime matches the selected
target profile. All evidence-boundary checksums verify at
`artifacts/experiment-0021-20260725T170433Z`.

No Steam, launcher, or ESO process was started by the agent.

## Result

Official MoltenVK 1.4.2 is now the installed maintenance baseline. It retains
Experiment 0020's established performance profile without adding diagnostic
hot paths or shader compression, and every prior runtime/cache state required
for rollback is preserved.

This result proves the requested installation and non-game compatibility
boundary. It does not claim that 1.4.2 fixes the known loaded-world
graphics-reset corruption. No dedicated user run is required for Experiment
0021; the next ordinary game session can serve as maintenance validation.

## 2026-08-01 ordinary-play validation amendment

The deferred ordinary-use boundary has now passed. The user reports extended
normal play at relatively high graphics settings, with no gameplay-limiting
performance problem. Unlike every prior 1.4.1 reset checkpoint, live resolution
and graphics-setting changes completed without persistent solid-color,
black-output, or frozen-frame rendering.

The latest preserved run is
`20260731T114051.034860000Z-pid30867`. Independent log facts are:

- automatic bridge-startup verdict: PASS;
- exact official MoltenVK 1.4.2 `performance-aggressive` configuration;
- six complete `DeviceWaitIdle` / swapchain / `OnDeviceReset` sequences;
- zero reset error markers;
- repeated `Systems Loaded And Not Waiting For Teleport` records;
- zero ESO crash reports after the run start.

The logs prove that resets occurred and completed without a recorded API error.
The user's direct observation supplies the fact that correct scene rendering
continued afterward. This is strong evidence that the former reset corruption
does not reproduce on the current 1.4.2 checkpoint, but it does not isolate a
specific upstream change or exclude an interaction with the fresh 1.4.2 cache.

One presentation artifact remains: a full-screen hot-pink frame appears for
roughly one second during early startup and then disappears. It has no reported
gameplay impact and persists with `SkipPregameVideos=1`, so the earlier video-
bypass hypothesis remains excluded.

The exact full settings file is preserved only in ignored evidence. Its
SHA-256 is
`470c9acaa599b61fabe8759c0089c69e31fb9723b34326c3949cd82db6a76382`;
the committed 48-key sanitized counterpart is
[`config/usersettings-m4-moltenvk-1.4.2-2026-08-01.txt`](../../config/usersettings-m4-moltenvk-1.4.2-2026-08-01.txt).
The active 1.4.2 pipeline cache is 7,754,395 bytes with UUID
`db6602241a0502090000000100000000` and SHA-256
`3bbd27ad1e37fce3981f8c7dc7dae419791149b147445d38cd9fab04d03413df`.

All raw checkpoint checksums verify at
`artifacts/experiment-0021-post-validation-20260801T052538Z`. The Experiment
0021 outcome is amended to **succeeded in ordinary high-settings gameplay**.

### 2026-08-01 performance clarification

The user clarifies that the on-screen counter remained at the 60 FPS VSync
ceiling throughout active gameplay in this roughly 93-minute session. The
validated 2048 x 1280 profile includes SSAO, high-resolution shadows, and the
other 48 allowlisted values in the committed standard settings asset. No
comparable sustained drop toward the embedded-runtime baseline's 30--33 FPS
state was observed.

The preserved ESO, interface, and bridge logs do not record continuous FPS
samples. The 60 FPS statement is therefore a direct user observation. The logs
independently establish the run identity and duration, exact 1.4.2
`performance-aggressive` activation, repeated loaded-world completion, six
graphics-device reset sequences, and no subsequent ESO crash report. This
supports the combined runtime/profile/settings/cache checkpoint as a successful
real-use performance and correctness result; it does not isolate a particular
MoltenVK change or guarantee the same result on another machine.
