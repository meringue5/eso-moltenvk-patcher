# Experiment 0006: Metal argument buffers disabled

- Date planned: 2026-07-19
- Outcome: **baseline lobby/world rendering succeeded; live SSAO toggle exposed
  a new solid-color rendering failure**
- Installation: **approved and performed from source commit `b2817da`**
- Rollback: **not performed; Experiment 0006 checkpoint installed**

## Question

Does disabling Metal argument buffers, while retaining every compatibility
control that passed Experiment 0005 startup, remove its black/shadow-layer
flicker and permit a short, render-correct world session through Steam?

## Basis and hypothesis

Experiment 0005 proved that both HDR filters are needed for startup. It loaded
MoltenVK 1.4.1, removed exactly one HDR device extension and one exact HDR
surface-format pair, avoided the NULL `vkSetHdrMetadataEXT` path, and terminated
normally. It nevertheless produced a transient full-screen hot-pink frame and
persistent high-frequency black/shadow-layer flicker at character selection.

MoltenVK 1.0.18 predates Metal argument-buffer support. MoltenVK 1.4.1 enables
it by default, and the captured ESO device enabled only `VK_KHR_swapchain`,
`VK_KHR_maintenance1`, and `VK_EXT_debug_marker`, not descriptor indexing.
Argument buffers directly change descriptor resource binding, making them the
narrowest next compatibility variable for the observed layer corruption.

The hypothesis is diagnostic, not a claim that the artifact is already fixed.
It is supported by the dated
[rendering-compatibility delta](../research/moltenvk-rendering-compatibility-delta.md).

## Exact change and controls

- Keep official pinned MoltenVK 1.4.1 and the same 17 byte-validated redirects.
- Keep `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1`.
- Keep the exact `VK_EXT_hdr_metadata` and HDR surface-pair filters.
- Set only `MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0` as the new variable.
- Keep MTLHeap at `where safe`, synchronous queue submits enabled, command
  pooling enabled, and command-buffer prefill disabled.
- Use marker mode `descriptor-compat`; reject every other marker value.
- Query the loaded runtime through `vkGetMoltenVKConfigurationMVK` before any
  patch is written. Fail closed unless all six controlled values match.

Do not change MTLHeap, queue submission, command pooling, fast math, user
graphics settings, or the HDR filters in this experiment.

## Non-game validation gate

Before installation:

1. Preserve and checksum-verify Experiment 0005 evidence.
2. Confirm ESO, Steam, and the launcher are stopped.
3. Restore only because the active proxy prevents a clean source rebuild;
   preserve the displaced marker and both pipeline-cache generations.
4. Fetch the official pinned runtime and rebuild from source.
5. Prove in separate processes that the packaged runtime reports argument
   buffers `1` under controlled defaults and `0` only in `descriptor-compat`
   mode, while the other controlled values remain unchanged.
6. Re-run proxy, self-patch, HDR filter, real Vulkan/Metal, wrapper-coverage,
   proc-route, Python, and shell checks.
7. Commit a clean source checkpoint, prepare ignored evidence, install through
   the experimental gate, and verify installed hashes and marker contents.

No agent launches Steam, the ESO launcher, or ESO.

## Completed pre-install validation

All 16 files in the final Experiment 0005 checksum manifest were reverified
before restoration. The installed run's 3,141,826-byte pipeline cache and its
SHA-256 `088eedb8bdce87ba23700866241b4b3280e69805bc4a45fd8946a873ebaaba0f`
were preserved as `PipelineCache.esopc.teso4m4-new-20260719-233729`. The prior
6,800,792-byte cache returned to the active name with its original SHA-256
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.
The Experiment 0005 marker was preserved as
`.teso4m4-enable.disabled-20260719-233729`.

Post-restore status reported the exact ESO fingerprint, original/inactive
loader, absent active marker, and identical active/pristine Bink SHA-256
`c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`.
Restoration was a clean-build prerequisite, not an operational goal.

The official MoltenVK 1.4.1 release was fetched and verified again. The clean
build passed Bink symbol re-export, Rosetta self-patching, fake HDR extension
and surface filtering, device tracing, and two independent configuration
probes. The packaged runtime reported:

| Process mode | Live resources | Argument buffers | MTLHeap | Sync submit | Command pooling | Prefill |
|---|---:|---:|---:|---:|---:|---:|
| controlled 1.4.1 default | 0 | 1 | 1 | 1 | 1 | 0 |
| `descriptor-compat` | 1 | 0 | 1 | 1 | 1 | 0 |

Fresh real Metal/Vulkan probes confirmed the embedded runtime still exposes
three sRGB surface formats and no ESO HDR pair. The rebuilt 1.4.1 runtime in
`descriptor-compat` mode exposed 60 raw formats, filtered exactly one pair to
59, and passed device creation with raw HDR advertisement hidden and a NULL
HDR setter on the non-HDR device route. MoltenVK identified descriptor binding
as discrete resource indexes, which is the intended non-argument-buffer path.

Fresh static analysis recovered 162 old Vulkan text symbols, 40 external
references to exactly the 17 manifest entries, and no referenced entry point
missing from 1.4.1. Proc analysis recovered 19 GIPA sites with 17 unique names
and 80 GDPA sites with 65 unique names. It found no unnamed site, missing probe
candidate, or old-nonnull/new-null regression on ESO's actual route.

Twelve startup-checker tests now fail closed on missing mode or effective
configuration evidence in addition to the HDR conditions. Python compilation,
shell syntax, and whitespace checks pass. Source commit
`b2817dad137e04e6558d213ad6a4796d8fb66de2` identifies the exact build.

## Installation checkpoint

The user-authorized objective includes preparing and installing the next
candidate without another intermediate pause. Immediately before installation,
process inspection found no ESO, Steam, or launcher process. Status reported
the known ESO fingerprint, original/inactive loader, and absent marker. The
worktree was clean; both the repository and ignored evidence identified source
commit `b2817dad137e04e6558d213ad6a4796d8fb66de2`.

The prepared artifact hashes were reverified before installation:

| Artifact | SHA-256 |
|---|---|
| proxy Bink | `fa8b89c2e090c36663f578e6a7d9fae557b0aebc6aa47a03ba4d9faeb3046550` |
| MoltenVK 1.4.1 | `d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398` |

Installation ran through the experimental gate in `descriptor-compat` mode at
approximately 23:44 KST. The prepared ignored evidence directory is
`artifacts/experiment-0006-20260719T144353Z/`.

Immediate post-install checks confirmed:

- exact ESO fingerprint, installed bridge, and present marker containing only
  `descriptor-compat`;
- active proxy and MoltenVK hashes identical to the prepared artifacts;
- pristine Bink unchanged at SHA-256
  `c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`;
- the 6,800,792-byte embedded-runtime cache preserved as
  `PipelineCache.esopc.teso4m4-old-backup`;
- the Experiment 0005 3,141,826-byte cache still preserved under its
  timestamped name, with no new active Experiment 0006 cache before launch;
- a clean source worktree.

No agent launched Steam, the launcher, or ESO. Installation is not runtime
evidence; the staged user run below remains the result gate.

## Exact user action after installation

This is one staged run, not two separate requests:

1. Launch ESO through the normal Steam and launcher path.
2. At character selection, observe normal animation for 30 seconds. Record
   whether the startup hot-pink frame and the black/shadow-layer flicker are
   each present or absent.
3. If persistent flicker, a black layer, a hang, or any other severe corruption
   is present, exit normally without entering the world.
4. If character selection is visually clean, select an existing character and
   enter the world immediately.
5. Allow 60 seconds for nearby assets to settle, then spend four minutes doing
   ordinary low-risk movement and camera rotation in the current area. Do not
   teleport, change settings, enable a HUD, or capture images.
6. Exit normally after five total minutes in the world, or stop immediately at
   a crash, hang, or visual corruption.
7. Report the two lobby observations, whether the world loaded, whether world
   rendering stayed correct for five minutes, and whether any crash reporter
   appeared only after normal exit.

The world stage is required if and only if the lobby is visually clean. This
lets a successful fix reach actual gameplay without exposing a known-bad lobby
configuration more broadly.

## Automatic and user-observed verdict

The bridge portion passes only if the selected run records:

- verified effective configuration with live resources `1`, Metal argument
  buffers `0`, MTLHeap `1`, synchronous submits `1`, command pooling `1`, and
  prefill `0`;
- MoltenVK 1.4.1 loaded and all 17 redirects activated;
- exactly one HDR extension and exact surface pair removed in count and data
  queries;
- successful device creation without `VK_EXT_hdr_metadata`;
- no `vkSetHdrMetadataEXT` query and no bridge error or fatal record.

The complete experiment passes only if the user also reports a visually clean
character-selection scene, successful world entry, correct world rendering,
and five stable minutes of ordinary movement. A transient pink startup frame
is recorded separately: persistence or recurrence fails correctness, while a
single pre-loading flash does not by itself establish whether gameplay is
usable.

## Rollback policy

Rollback is not an operational objective. Preserve the installed result as a
checkpoint unless restoration is required for evidence, comparison, rebuilding,
or the next controlled change. Never delete the pristine Bink backup, displaced
markers, pipeline caches, logs, or crash evidence.

## Follow-up boundary

If the lobby artifact remains, Experiment 0006 is a clean negative A/B and the
next candidates are MTLHeap and legacy asynchronous submission, tested one at a
time. If five-minute gameplay passes, preserve this configuration as the first
gameplay-capable MoltenVK 1.4.1 checkpoint before designing any performance
measurement.

## Run amendment: gameplay reached, SSAO transition failed

### Preserved evidence

The user launched through the normal Steam and launcher path. Run
`20260719T145051.899384000Z-pid35259` lasted from 23:50:51.899 to
23:54:43.454 KST, about 3 minutes 52 seconds. All 17 files in the final ignored
evidence manifest under `artifacts/experiment-0006-20260719T144353Z/` pass
checksum verification. The full post-run settings file was added to ignored
evidence before any corrective edit.

| File | SHA-256 |
|---|---|
| `bridge-log-after.txt` | `d9f2e7ef9b8c316abe65f6f2fa8a1fc87830940221437bc1bd56a128cf7c7337` |
| `startup-verdict.txt` | `b8acbfb8eeac07ae64a330d70ddf279592bf28497e6d83b658113b7309f8fb05` |
| `eso-client.log` | `60b2df871be71ef7585b39a32894b2ebaff7a6e6a725850c5a4fe5b3f90a20ec` |
| `eso-interface.log` | `e1d2c0ca3d7777669ee755407596a609ce56990181d867ad84ab794e8ce88f01` |
| `eso-unified.log` | `d14a39d5dee40828e3e328983059827006f2e05b3967e80987e5076dec70d886` |
| `UserSettings-after-ssao.txt` | `0ccfd0c6d30257454d495d0c74ba6b584a46609792d357f6499ca64c81690fab` |
| `SHA256SUMS` | `2dede3092ba3a804dd1b41efc929f393395903362d6a4b7b1bfeefc179622da9` |

The automatic bridge verdict was `PASS`:

- the effective runtime configuration was live-resource checking `1`, Metal
  argument buffers `0`, MTLHeap `1`, synchronous submits `1`, command pooling
  `1`, and prefill `0`;
- MoltenVK 1.4.1 loaded and all 17 redirects activated;
- both HDR filters removed exactly one item in count and data queries;
- the device was created with ESO's three legacy extensions and without HDR;
- ESO never queried `vkSetHdrMetadataEXT` and the bridge recorded no error;
- no new macOS crash report was created.

Although the user described the final action as a force quit, the system log
shows a Quit AppleEvent, one initially canceled termination request, a second
approved request, and `Termination complete. Exiting without sudden
termination.` The bridge also reached device, swapchain, surface, and instance
destruction. This was a render-correctness failure, not a process crash.

### Baseline rendering result

The transient hot-pink frame still appeared during the pre-game logo sequence.
The user did not regard it as a game-frame defect. More importantly, the
Experiment 0005 black/shadow-layer flicker was absent from character selection
through world entry, and normal world graphics were described as comfortable.

ESO entered character selection at 23:51:20.749, completed its character-select
textures at 23:51:28.641, began loading Auridon at 23:51:37.053, activated the
character at 23:51:43.769, and completed the load-screen transition at
23:51:45.601. The visually correct world interval lasted about 2 minutes 33
seconds before the later settings transition. The planned unchanged five-minute
interval was therefore not completed.

This single-variable A/B strongly implicates Metal argument-buffer use in the
Experiment 0005 flicker. It does not yet prove that every zone or extended
session is stable.

### Stuttering observation

The user observed stuttering during otherwise correct rendering. The unified
log contains 189 privacy-redacted Metal compiler warnings: 158 occurred during
the 23:50-23:51 startup and world-loading interval, with smaller bursts through
23:54. The new pipeline cache reached 3,983,422 bytes with SHA-256
`971293cca9a9ed748a894aa84aa60dc64bae5c590bb47da9dfed780039252d8c`.

This timing and cache growth are consistent with first-run shader/pipeline
compilation, but they do not prove that compilation caused each stutter. A
second unchanged run with this warm cache is the narrow test; no manual HUD or
capture is required.

### SSAO failure boundary

The user changed ambient occlusion from disabled to SSAO during live gameplay.
ESO then showed only changing solid colors while still responding to input.
The post-run settings file confirms `AMBIENT_OCCLUSION_TYPE "1"`; the preserved
baseline value was `0`.

At 23:54:18.513, the client recorded a 12.010 ms `DeviceWaitIdle`, followed by
swapchain creation and `OnDeviceReset` completion at 23:54:18.515. Metal
compiler warnings resumed immediately afterward. There is no logged Metal
command-buffer error, GPU address fault, device-lost result, bridge error, or
crash report. Presentation therefore appears to have continued while the scene
or post-processing composition became invalid.

The evidence does not yet distinguish these possibilities:

- SSAO's shader/render-pass path is incompatible with this runtime mode;
- the live graphics-device reset loses state that would be valid on a clean
  start;
- a new pipeline compiled or cached incorrectly during the transition.

Do not describe MoltenVK occlusion-query support as the cause; Vulkan occlusion
queries and screen-space ambient occlusion are different mechanisms.

### Revised next gate

The active settings file now contains SSAO `1`, so an uncontrolled relaunch may
start in the failed path. Do not ask the user to relaunch yet.

1. With explicit approval, preserve the current file and restore only
   `AMBIENT_OCCLUSION_TYPE` from `1` to the verified baseline `0` outside the
   game UI.
2. Re-run this installed bridge unchanged for five minutes in the same area,
   without changing graphics settings. This confirms the gameplay checkpoint
   and tests whether stuttering falls with the warm cache.
3. Treat any further SSAO test as a separate instrumented experiment. Capture
   pipeline-creation results and the device-reset boundary first, then compare
   clean-start SSAO with a live toggle. Do not change another MoltenVK control
   at the same time.

## Baseline-restoration amendment: 2026-07-20

Before any subsequent launch, all 17 files in the Experiment 0006 evidence
manifest were reverified. The live settings file still contained exactly one
supported `AMBIENT_OCCLUSION_TYPE "1"` line and still had SHA-256
`0ccfd0c6d30257454d495d0c74ba6b584a46609792d357f6499ca64c81690fab`.
The active pipeline cache was also unchanged at 3,983,422 bytes and SHA-256
`971293cca9a9ed748a894aa84aa60dc64bae5c590bb47da9dfed780039252d8c`.

With ESO, Steam, and the launcher absent, commit `59d455f`'s fail-closed helper
created timestamped backup
`UserSettings.txt.teso4m4-before-ao-1-to-0-20260720T115609Z` and atomically
changed only that setting to `0`. The backup hash equals the preserved
post-SSAO hash above. The updated file hash is
`e71a11c20828ad270c5260b648cd6adb5cd2a7a58be6677acb4a5d5ec3ed49a2`;
substituting its AO line back to `1` reproduces the backup hash, independently
confirming that no other settings line changed.

Post-edit status still reports the Experiment 0006 bridge installed and its
enable marker present. The active pipeline-cache size, timestamp, and hash are
unchanged. No game-bundle file or cache was restored. The checkpoint is now
safe to advance to the unchanged five-minute, SSAO-disabled warm-cache repeat;
it is not yet a general gameplay approval.

## Client-update amendment: 2026-07-20

The user opened the Steam-authenticated launcher for the prepared warm-cache
repeat, but no ESO process was launched. Before the Play action, the launcher
installed client 12.0.7 and replaced the executable fingerprint. The prepared
repeat evidence directory is marked invalidated by the update and must not be
collected or interpreted as a runtime result.

The installed Experiment 0006 proxy still identifies the superseded target and
will fail closed on the new UUID. Experiment 0006's completed observations and
warm pipeline cache remain preserved; the next launch moves to Experiment 0007
after target rebase, clean rebuild, and non-game validation.
