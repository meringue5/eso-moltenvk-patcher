# Experiment 0013: command pooling disabled during live reset

- Date: 2026-07-25
- Outcome: **failed; solid-color live-reset corruption persisted**
- Rollback: **not performed; restore path checked before installation**

## Question

Does MoltenVK internal command-object reuse retain invalid descriptor/resource
state across ESO's loaded-world graphics reset?

## Basis

Experiment 0012 reproduced solid-color output while eight command buffers,
484 balanced render passes, and 7,699 indexed draws were submitted without a
Vulkan failure. The same bounded interval allocated and freed no Vulkan command
buffer, but destroyed 77 image views, created 67, and made 93,707 descriptor
updates. Official MoltenVK 1.4.1 source places the leading compatibility delta
in descriptor/resource state.

MoltenVK documents `MVK_CONFIG_USE_COMMAND_POOLING=0` as allocating and
destroying internal command memory for each command instead of reusing it.
This is the narrowest available single-variable counterfactual for stale
command/resource state. It may reduce CPU performance.

## Exact change and controls

- Keep official MoltenVK 1.4.1 and all 17 validated redirects.
- Keep both HDR filters, live-resource checking, disabled Metal argument
  buffers, synchronous queue submission, command-buffer prefill disabled, and
  MTLHeap `where safe`.
- Keep the observation-only lifecycle and bounded reset-resource traces.
- Change only `MVK_CONFIG_USE_COMMAND_POOLING` from `1` to `0`.
- Require the effective post-load configuration query to prove the six
  controlled values before any ESO patch is applied.

## Non-game gate

1. Pass warnings-as-errors, static analysis, all smoke probes, Python tests,
   shell syntax, and whitespace checks.
2. Verify default, descriptor-compatible, legacy-allocation,
   reset-resource-trace, and no-command-pooling effective configurations in
   separate processes.
3. Rebuild from committed source against the pristine game loader.
4. Re-run both real-Metal compatibility probes.
5. Obtain explicit approval before modifying the game bundle.

## User action

Installation is complete. One Steam-authenticated run will:

1. enter the existing character's world and confirm initial rendering;
2. change fullscreen resolution once from 1920 x 1200 to any other available
   value;
3. stop immediately after correct rendering or persistent corruption appears.

No HUD, capture, FPS report, travel, or unrelated graphics change is required.

## Pass/fail

- **Pass:** at least eight replacement-swapchain frames render correctly after
  the reset and the bounded trace completes without a Vulkan failure.
- **Fail:** persistent solid-color/frozen output recurs, a Vulkan failure is
  logged, or ESO crashes.

A pass implicates pooled internal command/resource state but is not yet a final
performance configuration. A fail moves the next instrumentation directly
into descriptor contents and lifetimes.

## Validation result

Source commit `ecec1cf` adds the distinct `no-command-pooling` marker, enables
the existing bounded trace in that mode, and refuses to patch unless the
queried MoltenVK configuration reports command pooling `0` with every other
controlled value unchanged.

Fifty-six Python tests, Python compilation, shell syntax, whitespace checks,
warnings-as-errors, and Clang static analysis passed. The complete temporary
pristine-loader build passed Bink re-export, Rosetta self-patch, HDR,
lifecycle, reset-resource, and all five effective-configuration probes. The
new probe reports:

```text
mode=no-command-pooling result=0 size=184 live_resources=1
metal_argument_buffers=0 use_mtlheap=1 synchronous_queue_submits=1
command_pooling=0 prefill=0
MoltenVK configuration probe: PASS
```

Both fresh real-Metal probe pairs reached the Apple M4. The replacement runtime
retained its expected HDR advertisement and exact 60-format list, while the
embedded runtime retained no HDR advertisement and its three-format list.
Device creation and the 100-name proc comparison completed for both runtimes.

The prepared proxy currently has SHA-256
`3a0865f9a43c6629f71859a64e96eff24a18c9847dc59c42f14b965e2fb5cda3`;
the replacement MoltenVK remains
`d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398`.
These hashes were reproduced by a second complete build from committed source
against a temporary app structure pointing at the pristine loader. No
game-bundle file was changed.

At the source-validation checkpoint, the Experiment 0012 bridge remained
installed. No agent launched Steam, the launcher, or ESO, and no game-bundle
file was modified while preparing the candidate.

## Installation

The user directed that prepared experiments be installed without another
installation question, explicitly authorizing Experiment 0013 in this turn.
The target and content were current, the worktree was clean, and exact process
checks found ESO, Steam, and the launcher stopped.

A cache-preserving restore returned the loader to the pristine source without
moving either pipeline cache. Commit `31b2c65` was then rebuilt against the real
bundle, passed the complete build gate again, and was installed in
`no-command-pooling` mode. Installed and built hashes match:

```text
libBink2Macx64.dylib
3a0865f9a43c6629f71859a64e96eff24a18c9847dc59c42f14b965e2fb5cda3

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

The marker contains exactly `no-command-pooling`, the target is current, and
the quick gate is `READY`. Fresh evidence is prepared under the ignored
`artifacts/experiment-0013-20260725T121340Z` directory.

The exact 1920 x 1200 settings file, ambient occlusion `1`, pregame-video
setting `1`, 4,203,757-byte active cache, and 6,800,792-byte old cache were
preserved. No agent launched Steam, the launcher, or ESO.

## Runtime result

The user entered the world with initially correct rendering and changed
fullscreen resolution once from 1920 x 1200 to 2048 x 1280. Persistent
solid-color output recurred. The settings diff is structurally exact and
contains only those two dimension changes.

The bridge proved the intended effective configuration, including command
pooling `0`, and the automatic startup verdict passed. No crash report, Vulkan
failure record, device-loss marker, focus loss, or reset error was observed.
All evidence checksums pass.

Replacement swapchain generation 3 acquired and presented 262 frames. Its
first eight frames contained eight submitted command buffers, 484 balanced
render passes, 1,565 pipeline binds, 9,671 descriptor-set binds, and 9,671
indexed draws. The reset successfully created 151 graphics pipelines, 53
images, 55 image views, 35 buffers, 50 render passes, and 50 framebuffers.

No Vulkan device memory or command buffer was allocated, freed, mapped, or
unmapped in the bounded window. Fifteen complete captured image bindings were
aligned and non-overlapping. The reset made 131,851
`vkUpdateDescriptorSets` calls. The active pipeline-cache size remained
4,203,757 bytes but its content hash changed; the old cache remained
byte-identical.

## Interpretation

Disabling MoltenVK internal command pooling does not repair the failure.
MoltenVK command-resource reuse is therefore excluded as a primary cause.
This does not exclude ESO re-recording or resubmitting its already allocated
Vulkan command buffers.

Together with Experiments 0010 through 0012, the result places the leading
fault after successful offscreen resource creation and before or within the
final composite pass. The strongest specific hypothesis is that a descriptor
used by that pass still names a destroyed image view, fails MoltenVK 1.4.1's
live-resource check, or is not rebound after the reset. The next source work
must track descriptor contents and image-view lifetimes by handle and correlate
them with command-buffer binds. It must also trace image barriers and render
pass attachment layouts to distinguish a stale descriptor from an undefined
image-layout transition. No further generic configuration experiment is
justified first.
