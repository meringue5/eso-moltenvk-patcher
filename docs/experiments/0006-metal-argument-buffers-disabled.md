# Experiment 0006: Metal argument buffers disabled

- Date planned: 2026-07-19
- Outcome: **pending**
- Installation: **not yet performed**
- Rollback: **Experiment 0005 checkpoint remains installed until restoration is
  required for this clean rebuild**

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
shell syntax, and whitespace checks pass. Installation remains pending until a
clean source commit and ignored evidence directory identify the exact build.

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
