# Project status

Last updated: 2026-07-25

## Safety state

The experimental MoltenVK bridge is validated only for a short world interval
with ambient occlusion disabled; it is not yet a general gameplay build.
Experiments
0001 and 0002 activated MoltenVK 1.4.1 and then crashed during early graphics
startup. Experiment 0002 added complete proc tracing and is documented in its
[experiment record](experiments/0002-live-check-proc-trace-startup.md).

Experiment 0004 was explicitly approved, rebuilt from source commit `e8ee4d2`,
installed in `live-check` mode, and launched by the user through Steam. It
successfully filtered `VK_EXT_hdr_metadata`, recorded a device created without
that extension, and still crashed immediately at the confirmed NULL HDR setter.
See its [completed record](experiments/0004-hdr-advertisement-filter-startup.md).

After preserving the failed Experiment 0004 checkpoint and re-verifying all 14
evidence-file checksums, restoration became technically necessary for the
Experiment 0005 clean rebuild. The user stopped Steam and the launcher, and
`scripts/restore.sh` ran at approximately 2026-07-19 22:40 KST. The
non-destructive `scripts/status.sh` check after restoration reports:

- the analyzed ESO executable still matches the fingerprint in the
  [target manifest](../config/targets-eso-2026-07-11.json);
- the active Bink loader is original and the bridge is inactive;
- the enable marker is absent;
- the active and pristine Bink SHA-256 values both equal
  `c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`.

This is a point-in-time observation, not a persistent guarantee. Run the status
check again before any work involving the game bundle. Inactive companion files
may exist beside the game executable; the status result above does not inventory
or remove them.

The user directed that rollback is not an operational goal. The failed state
was preserved until restoration became necessary for a clean rebuild. Its raw
evidence, displaced marker, and pipeline-cache state remain preserved; the
current original-loader state is a build prerequisite, not an assertion that
normal operation was the objective.

At 2026-07-19 22:54 KST, the user approved continuing directly with the
prepared Experiment 0005 installation. Pre-install checks found no ESO, Steam,
or launcher process; required original/inactive status and exact source commit
`b00ced5d521c301a1bc159b74cfa16056a5c5e36`; and reverified both prepared
artifact hashes. The bridge was then installed in `live-check` mode. Immediate
post-install status reports the bridge installed and marker present. The active
proxy and MoltenVK hashes match the prepared evidence, the pristine Bink remains
unchanged, and the 6,800,792-byte old pipeline cache is preserved under its
backup name.

The user ran Experiment 0005 through the normal Steam path from 23:04:28 to
23:05:44 KST, about 76 seconds. The automatic bridge verdict passed and no new
crash report was created. The run reached character selection and exited
normally, but rendering correctness failed visibly: startup briefly showed a
full-screen hot-pink frame, and character selection showed high-frequency
flicker in a black layer described as shadow-like. The installed checkpoint is
being preserved for analysis; it is not approved for world entry or gameplay.

After all 16 Experiment 0005 evidence checksums were reverified, restoration
became technically necessary for the Experiment 0006 source rebuild. At
approximately 23:37 KST, `scripts/restore.sh` restored the pristine loader and
the prior pipeline cache. The Experiment 0005 marker and its new cache remain
preserved under timestamped names. Post-restore status reports the exact ESO
fingerprint, original/inactive loader, absent active marker, and byte-identical
active/pristine Bink files. This is a build checkpoint, not an operational
rollback objective.

The user then ran Experiment 0006 through Steam from 23:50:51 to 23:54:43 KST.
The bridge verdict passed, no crash report was created, and the user reached
character selection and Auridon without the Experiment 0005 black/shadow-layer
flicker. World rendering remained visually correct for about 2 minutes 33
seconds. The user then changed ambient occlusion from disabled to SSAO, after
which the display became changing solid colors despite continued input
response. The process ultimately completed an orderly AppKit and Vulkan
teardown. All 17 ignored evidence files and the post-run settings file are
checksum-preserved in the Experiment 0006 directory. The bridge remains
installed; no rollback was performed.

At 20:56 KST on 2026-07-20, after all 17 evidence checksums and the live
settings and cache fingerprints were reverified, the active settings file was
backed up and only `AMBIENT_OCCLUSION_TYPE` was changed from `1` to `0`. The
backup SHA-256 is the preserved post-SSAO hash
`0ccfd0c6d30257454d495d0c74ba6b584a46609792d357f6499ca64c81690fab`;
the updated file hash is
`e71a11c20828ad270c5260b648cd6adb5cd2a7a58be6677acb4a5d5ec3ed49a2`.
Transforming that one updated line back to `1` reproduces the backup hash,
which verifies that no other setting changed. The installed bridge, enable
marker, and 3,983,422-byte pipeline cache remained unchanged.

The planned warm-cache repeat was interrupted before ESO launched. Opening the
launcher at approximately 21:02 KST installed launcher-managed client 12.0.7,
databuild `3281538`. The ESO executable changed to SHA-256
`82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`
and UUID `6599A49F-B1A0-3CBF-9AEF-6D4186A66E0D`. The prepared repeat evidence
directory is explicitly marked invalidated; it is not a runtime result. No ESO
process was launched.

At that checkpoint, the active loader remained the Experiment 0006 proxy, but
it still targeted the superseded executable and would fail closed on the UUID
mismatch. The launcher expanded the active proxy file from 44,400 to 226,368
bytes; its first 44,400 bytes remained byte-identical to the prepared proxy.
The enable marker and warm pipeline cache remained present. That state was
preserved until the clean rebuild below and was not approved for launch.

After Steam and the launcher exited, a cache-preserving restore returned only
the loader to the pristine Bink while leaving both pipeline caches in place.
Source commit `fa88c6e` rebuilt against the new manifest and passed the full
non-game gate. At approximately 21:19 KST, the rebased Experiment 0007 bridge
was installed in `descriptor-compat` mode. Post-install status reports the new
ESO target recognized, bridge target current, and enable marker present. The
active 3,983,422-byte warm cache, 6,800,792-byte old backup, and SSAO-disabled
settings are unchanged.

The user then launched Experiment 0007 normally through Steam at 21:24 KST.
The approximately 47-second run passed the automatic bridge-startup verdict:
MoltenVK 1.4.1 loaded, all 17 redirects activated, the HDR extension and
surface filters returned 130 of 131 and 59 of 60 entries respectively, device
creation succeeded without HDR, teardown was orderly, and no bridge error,
Metal compiler warning, or new crash report appeared. The user observed the
same transient hot-pink startup splash, but server maintenance prevented entry
to character selection and the world. Gameplay rendering and stuttering are
therefore untested, not failed. The active cache kept its 3,983,422-byte size
but its content hash changed; this alone does not identify shader compilation
or prove a warmed world cache. The bridge remains installed and current, SSAO
remains disabled, and no rollback was performed.

The launcher-update boundary is now automated for future runs. A local checker
compares executable SHA-256 and UUID against the selected and historical target
manifests, and evidence preparation fails closed on any mismatch. The current
12.0.7 manifest has a schema-v2 profile containing both embedded MoltenVK
object hashes, all 17 exact patch signatures, the 40-reference shape, the
19-site GIPA and 80-site GDPA query shapes, and the pinned replacement-runtime
identity. A fast rebase tool may create and select a new manifest only if that
entire profile remains exact; it never modifies the game bundle. This tooling
does not itself establish runtime rendering or stability.

After service returned on 2026-07-21, the user completed a short Experiment
0007 world repeat. The automatic bridge verdict passed, no new crash report or
bridge error appeared, and ESO's interface log reached character selection and
fully loaded Auridon. The approximately 199-second process contained about 162
seconds after world load. The user explored a limited area, observed correct
rendering, and did not perceive stuttering. The active cache kept its
3,983,422-byte size but changed content hash again; the settings file remained
byte-identical with ambient occlusion disabled. The originally planned
five-minute interval was not completed, so extended stability and controlled
performance remain unproven.

The transient solid-color startup screen persisted. ESO's own interface log
places it in the pre-UI sequence beginning with `PlayIntroMovies`, followed by
ZOS video, Havok, and legal splash states. The executable recognizes the
supported `SkipPregameVideos` key, whose value was `0` in every observed run.
Experiment 0008 preserved the exact settings file and changed only that value
to `1`; changing the line back in memory reproduces the original hash. The
bridge, cache, ambient occlusion, and all other inspected video settings remain
unchanged.

At 18:51 KST on 2026-07-25, the launcher applied a 308,741,853-byte
self-update. Its post-restart `Live_Prod` check took the
`noUpdateRequired` path and reported matching local and remote IDs for all
eight repositories. No ESO game-bundle file crossed that modification-time
boundary. The game remains client 12.0.7, databuild `3281538`, with the exact
selected executable hash and UUID; the installed bridge is still current and
enabled. Experiment 0008's settings hash is unchanged with pregame videos
skipped and ambient occlusion disabled. This is a launcher-only update, not an
ESO target rebase, and no user runtime test occurred. The raw launcher logs
were checksum-preserved outside Git; fresh preflight evidence is required
before the pending test.

The update tooling now has a second passive checker for this distinction.
`scripts/check-launcher-state.sh` accepts only a completed, full-coverage
launcher `Live_Prod` state check, requires its `noUpdateRequired` path, and
compares every recorded local and remote repository ID. Later lightweight
checks do not displace the last full snapshot. It neither launches an app nor
contacts the network. Its result is point-in-time evidence and does not replace
the exact executable/UUID gate or predict a future remote update.

Routine checks now use `scripts/quick-update-check.sh`, which combines the
exact ESO identity and client build, the last full launcher repository
comparison, and the installed bridge target/marker into one `READY` or `STOP`
result. The complete gate measured approximately 0.15–0.16 seconds on five warm
runs on this checkpoint. It rejects a full launcher snapshot older than one
hour by default. Evidence preparation and collection invoke the same coded
gate and preserve repository IDs plus the relevant setting hashes and values.
A `READY` result ends routine analysis; only `STOP` or contradictory local
evidence opens the slower forensic or rebase path.

The slower exact-layout audit now profiles the client version/databuild as
well, checks that stamp for an in-flight update race, and verifies it again
before target selection. A real local audit reproduced the full static profile
and client build in 53.8 seconds without modifying the game bundle. This path
is reserved for `STOP`; it is not part of the 0.15–0.16-second routine gate.

The user then launched at 19:07 KST on 2026-07-25 through the normal Steam
path. Experiment 0008's video bypass took effect: the interface log began at
`AccountLogin` with no logged pregame-video or logo states, but the transient
hot-pink frame still appeared. Startup, character selection, and Auridon
otherwise rendered correctly. The run predates its final evidence-preparation
timestamp by about two minutes, so the authoritative time-gated verdict
correctly has no eligible run. A separately labelled retrospective verdict
selects the exact 19:07 run and passes all 17 redirects, MoltenVK 1.4.1
descriptor compatibility, HDR-extension and surface-format filtering, and
HDR-disabled device creation.

During that same run, lowering fullscreen resolution from 2048 x 1280 to
1920 x 1200 produced persistent solid-color output. The client log places
`DeviceWaitIdle`, swapchain recreation, and `OnDeviceReset` immediately before
the symptom, with no device-loss marker, bridge error, new `.ips` report, or
process crash. Exact before/after settings, logs, caches, and a 35-file checksum
manifest are preserved under the ignored Experiment 0008 evidence directory.
The active lower resolution is preserved and has not been cold-start
validated; no rollback was technically necessary.

Experiment 0002 was explicitly approved, installed from source commit
`7a235dc`, run by the user, collected, and rolled back. Immediately after that
rollback, the then-active loader byte-matched the pristine backup. Raw evidence
and its checksum manifest remain under the ignored `artifacts/` directory; they
must not be committed.

Experiment 0003 retrospectively examined a user-controlled session lasting
about 2 hours 27 minutes. Its loaded-image list contains only the original Bink
UUID and no bridge or dynamic MoltenVK image. The session therefore used the
embedded MoltenVK 1.0.18 runtime; it is not evidence that the override works.
Its current settings, updated pipeline cache, and exit report are preserved as
an ignored baseline checkpoint. See the
[Experiment 0003 record](experiments/0003-original-runtime-long-session.md).

## Active blocker

The 12.0.7 target rebase now has positive startup, lobby, and short-world
evidence. Static analysis found that the bundled MoltenVK object is
byte-identical to the prior object, its headers still identify 1.0.18, the link
delta remains `0x1a08490`, and the same 40 external references reach the same
17 wrappers. All 17 patch offsets and their 12-byte preconditions are
unchanged. The former GIPA/GDPA slots still recover 19 and 80 direct query sites
respectively, with no unnamed site, missing probe candidate,
manifest-coverage gap, or old-nonnull/new-null regression. Fresh real-Metal
probes reproduced the expected HDR extension and surface filtering, and the
maintenance-interrupted launch and the later world repeat reproduced those
filters in ESO. The target rebase itself is no longer the active blocker.

Experiment 0004 falsified the device-advertisement-only hypothesis. The bridge
removed exactly one HDR device extension, and ESO still queried and called the
NULL setter. In both Experiments 0002 and 0004, Rosetta `tmp1` equals the
ASLR-adjusted instruction immediately after ESO's indirect
`vkSetHdrMetadataEXT` call, confirming that call as the fault site.

The former startup blocker is resolved for one controlled run. ESO enables its
HDR surface flag when surface enumeration includes format `64` with color space
`1000104008`.
Embedded MoltenVK 1.0.18 reports three sRGB formats and no matching pair;
MoltenVK 1.4.1 reports 60 formats and includes exactly one. A source-only
wrapper removed that pair from both ESO count and data queries, produced 59
visible formats, and prevented any `vkSetHdrMetadataEXT` lookup. ESO continued
through swapchain, pipeline, draw, present, and orderly teardown calls.

Disabling Metal argument buffers removed the Experiment 0005 persistent
black/shadow-layer flicker in one single-variable A/B and allowed world entry.
This strongly implicates the argument-buffer descriptor path. A second short
world interval has now repeated the rendering pass; an unchanged five-minute
interval is still missing.

The active rendering blocker is now the loaded-world live reset path rather
than SSAO alone. Experiment 0009 lowered fullscreen resolution from
2048 x 1280 to 1920 x 1200 with ambient occlusion still disabled. The display
immediately became persistent solid colors after `DeviceWaitIdle`, swapchain
recreation, and `OnDeviceReset`, matching the high-level Experiment 0006 SSAO
sequence. No current-run bridge error, device loss, HDR setter lookup, new
`.ips` report, or process-level crash appeared. The resolution-only trigger
strongly shifts the working hypothesis toward reset/resource recreation, but
does not identify the failing resource or owner.

MoltenVK 1.4.1 performance A/B testing remains blocked until rendering
correctness and short gameplay stability are established.

The Experiment 0003 process ended with `EXC_BAD_INSTRUCTION / SIGILL` in an
audio teardown thread after otherwise usable gameplay. This is a separate
shutdown failure from the bridge startup crash.

## Performance baseline

The user reported generally higher FPS in Experiment 0003, object-heavy areas
dropping into the 40s, no visible improvement from changing graphics options in
that state, and occasional recovery to about 60 FPS without logout. These are
valuable observations but not a controlled old/new A/B: no paired GPU-time,
memory, thermal, camera, population, or before-session cache measurements were
captured. They must not be attributed to MoltenVK 1.4.1.

Rust tooling is now locally available as `rustc 1.97.1` and `cargo 1.97.1`,
with both Apple Silicon and x86_64 macOS targets installed. No project component
depends on Rust yet.

## Next gate

Experiment 0010's observation-only source candidate is now statically verified.
It traces device waits; swapchain generations; their images, image views,
render passes, and framebuffers; and the first eight acquires/presentations per
generation. Its forwarding smoke probe passes, all 42 Python tests pass, and
Clang reports no warning or static-analyzer finding. It does not change the
17-entry redirect set, compatibility filters, Vulkan inputs, or results.

The user approved the cache-preserving technical restore and installation.
After Steam stopped, the pristine-loader restore, full source build, all
non-game smoke checks, current/legacy real-Metal probes, and installation
completed. Installed proxy and MoltenVK hashes exactly match the built
artifacts. Both pipeline caches and the exact 1920 x 1200 settings state are
unchanged. Post-install status is current and enabled, the quick update gate is
`READY`, and fresh ignored evidence is prepared.

The Experiment 0010 cold start reached a correctly rendered world. A later live
reset created and used a third swapchain generation with clean tracked
image/view/render-pass/framebuffer lifetimes, but output still failed.
Promoting suboptimal present or acquire results to out-of-date did not make ESO
recreate the swapchain. Compatible stretch scaling removed every suboptimal
result while solid-color output persisted. Those result-changing probes were
removed, and the observation-only checkpoint was rebuilt and reinstalled.

The next gate is Experiment 0011: retain MoltenVK 1.4.1, both HDR filters,
live-resource checking, disabled argument buffers, synchronous submission,
command pooling, and lifecycle tracing, while changing only MTLHeap from
`where safe` to `never`. One live resolution change will determine whether
Metal-heap allocation/reuse is causal. Performance A/B remains downstream of
rendering correctness.

Experiment 0011 is now installed from source commit `a354d9e` in the distinct
`legacy-allocation` marker mode. Default, descriptor-compatible, and
legacy-allocation probes reported MTLHeap `1`, `1`, and `0` respectively; all
non-game and real-Metal gates passed. Installed artifact hashes match the
build, both pipeline caches and user settings were preserved, the bridge target
is current, and the quick update gate is `READY`. Fresh ignored evidence is
prepared. No ESO, Steam, or launcher process was started by the agent.

The user-controlled Experiment 0011 run failed rendering correctness. ESO
verified MTLHeap `0`, and the settings changed only from 1920 x 1200 to
2048 x 1280. The terminal live reset completed without an API error, then
generation 3 acquired and presented 382 frames while the user saw persistent
solid-color output. Tracked swapchain resource lifetimes were clean; no bridge
error, device loss, focus loss, crash, or new report occurred. MTLHeap is
therefore excluded as the cause of this failure and must return to `where safe`
in the next candidate.

The next gate is not another configuration flag. Prepare observation-only,
reset-window-bounded tracing for offscreen images and memory, descriptors,
pipelines, command buffers, and queue submissions. Installation of that
candidate remains a separate experiment gate.

Experiment 0012's source and non-game gates now pass from commit `a1714da`.
The tracer covers both direct patch targets and GDPA results, remains inactive
until the post-generation-2 device wait, closes after eight replacement
presentations, caps detail logs, and preserves all arguments and results. Its
chained smoke probe, analyzer tests, complete temporary pristine-loader build,
four effective-configuration probes, and fresh real-Metal compatibility probes
passed. At that validation checkpoint the failed Experiment 0011 bridge
remained installed and Experiment 0012 had not modified the bundle.

The user approved Experiment 0012 installation. A cache-preserving restore,
second full source build, installation in `reset-resource-trace` mode, artifact
hash comparison, target/status check, and quick update gate all passed. MTLHeap
is restored to `where safe`; settings and both pipeline caches are unchanged.
Fresh ignored evidence was prepared. The user-controlled resolution change
from 2048 x 1280 to 1920 x 1200 again produced persistent solid-color output.
The eight-presentation trace bound is only a diagnostic capture window, not a
recurrence of the resolved one-run 8 FPS symptom.

The bounded trace completed with no Vulkan failure while eight command buffers,
484 balanced render passes, 7,699 indexed draws, and 7,699 descriptor-set binds
were submitted. It successfully created 119 graphics pipelines, 65 images, 67
image views, 53 buffers, 50 render passes, and 50 framebuffers. The renderer
therefore continued producing work; this is not a command-starvation or
swapchain-presentation failure.

No device memory was allocated, freed, mapped, or unmapped in the trace
window. All new buffers and images reused existing allocations. A coded
post-analysis found the 15 complete captured image bindings aligned and
non-overlapping. The reset destroyed 77 image views, created 67, allocated 456
descriptor sets, and invoked `vkUpdateDescriptorSets` 93,707 times. Official
1.4.1 source confirms that the enabled live-check mode skips a descriptor
target when its Metal resource is no longer live, while 1.0.18 uses the older
direct Metal-resource binding model. Descriptor/resource state is now the
leading compatibility boundary, although a specific broken descriptor is not
yet proven.

The next gate is a single-variable command-pooling counterfactual. Retain
MTLHeap, disabled argument buffers, both HDR filters, live checking,
synchronous submission, and the reset trace; set only
`MVK_CONFIG_USE_COMMAND_POOLING=0`. Experiment 0012 submitted eight command
buffers without allocating or freeing one in the reset window, so this tests
whether pooled command/resource state survives ESO's live reset incorrectly.
It is a correctness test with a possible CPU-performance cost, not a proposed
final performance configuration.

Experiment 0013's source commit `ecec1cf` now passes 56 Python tests, Python
compilation, shell syntax, whitespace, warnings-as-errors, Clang static
analysis, the complete pristine-loader build, all five effective-configuration
probes, and fresh legacy/replacement real-Metal probes. Its distinct
`no-command-pooling` mode reports command pooling `0` while preserving
live-resource checking `1`, argument buffers `0`, MTLHeap `1`, synchronous
submission `1`, and prefill `0`. The installed Experiment 0012 bridge and the
game bundle were not changed during this preparation. A second full build from
that committed state reproduced the prepared proxy and replacement-runtime
hashes.

The user explicitly authorized immediate Experiment 0013 installation. Exact
process checks found ESO, Steam, and the launcher stopped. A cache-preserving
restore, real-bundle rebuild from commit `31b2c65`, installation in
`no-command-pooling` mode, installed/built hash comparison, exact marker check,
status check, and quick update gate all passed. Both pipeline caches, the exact
1920 x 1200 settings file, ambient occlusion `1`, and pregame-video setting `1`
were preserved.

Fresh ignored evidence is prepared under
`artifacts/experiment-0013-20260725T121340Z`. The next gate is one
user-controlled world entry and one fullscreen-resolution change. No other
setting, travel, HUD, capture, or performance report is required.

That user-controlled run failed rendering correctness. The exact settings
change was 1920 x 1200 to 2048 x 1280, and persistent solid-color output
recurred with command pooling verified at `0`. Generation 3 acquired and
presented 262 frames. The bounded trace submitted 9,671 indexed draws and
descriptor-set binds, created all tracked resources successfully, and recorded
no Vulkan failure. No crash report or focus loss occurred. Experiment 0013
therefore excludes MoltenVK internal command pooling as the primary cause.

The leading fault region is now the connection between recreated offscreen
images and the final composite pass. Ranked hypotheses are:

1. a final-pass descriptor retains a destroyed image view or is skipped by
   MoltenVK 1.4.1's live-resource check;
2. a descriptor update/rebind or application command-buffer ordering pattern
   tolerated by 1.0.18 is not reflected by 1.4.1's descriptor state tracker;
3. an old/new offscreen-image lifetime or synchronization hazard survives
   `vkDeviceWaitIdle` when ESO reuses existing `VkDeviceMemory`;
4. an image barrier, attachment layout, or load/store transition leaves the
   final composite source undefined;
5. a reset-created pipeline or pipeline-cache key is incompatible with the new
   render-target dimensions.

Swapchain lifecycle, presentation result/scaling, HDR, focus, MTLHeap,
argument-buffer enablement, and MoltenVK command pooling are not leading
causes. Stop generic configuration toggles. Before another user run, implement
handle-level descriptor/image-view lifetime tracking plus barrier and
attachment-layout tracing, and use that evidence to prepare a repair
counterfactual rather than another blind setting A/B.

Experiment 0014 now bundles the five remaining categories into one
observation-only render-graph audit candidate. A descriptor/image-view mirror
starts at the first swapchain; the existing eight-presentation reset window
then joins stale descriptor binds, live image-memory overlap, destroyed-range
memory reuse, image barriers, render-pass attachment layouts, reset-created
pipeline linkage, and image copy/blit/resolve calls by sequence. The first 128
passes retain their tail pipeline and descriptor-set state so per-draw binds
cannot consume the useful log budget. Fixed-table overflow fails the analyzer
instead of silently reducing coverage. Command pooling returns to its 1.4.1
default.

Source commit `28d9bce` passes 61 Python tests, Python compilation, shell
syntax, whitespace, warnings-as-errors, Clang static analysis, the complete
pristine-loader build, reset and render-audit probes, and all six effective
configuration probes. The repeated hashed descriptor update path measured
33 nanoseconds per call in the x86_64 probe, well below its 10-microsecond
rejection threshold. Fresh replacement and embedded-runtime probes both
reached the Apple M4 and reproduced their expected HDR, surface-format, device,
and proc-address differences.

Experiment 0014 is installed in exact `render-audit` mode. Built and installed
proxy hashes match, the replacement MoltenVK hash is unchanged, the target and
marker are current, and the quick gate is `READY`. Command pooling is verified
back at `1`; settings and both pipeline caches were preserved. Fresh ignored
evidence is prepared under
`artifacts/experiment-0014-20260725T125558Z`. The next gate is one
user-controlled world entry and one fullscreen-resolution change, stopping as
soon as correct output, solid color, a frozen last frame, or a crash is known.
No HUD, capture, travel, FPS measurement, or second setting change is needed.

The user then exposed two states under this candidate. An initial 8 FPS phase
allowed resolution and other option changes without corrupting output. After a
complete Steam-path exit and restart, performance returned to 60 FPS, but one
resolution change again produced persistent solid color. Four audits began;
three completed without Vulkan failure or mirror overflow. The terminal
60 FPS/fail run completed 484 render passes and 9,671 indexed draws in its
first eight replacement frames.

Across all three completed audits, descriptor slots still referenced views at
view destruction, but no stale set was subsequently bound. There were no
unknown handles, live image overlaps, tracked layout mismatches, dead
attachments, or pipeline/render-pass mismatches. Destroyed image-range reuse
was more frequent in the earlier completed runs than in the terminal corrupt
run, so its count alone is not causal. The simple stale-view rebound and public
layout/linkage hypotheses are substantially reduced.

Every reset graphics pipeline in every completed audit was created with the
active pipeline cache. The cache grew by 55,314 bytes and changed hash over the
multi-run session, while the old backup was unchanged. Combined with the
8 FPS/reset-correct versus restarted 60 FPS/reset-fail split, this makes a
reset-only null pipeline-cache counterfactual the next gate. It is a targeted
Vulkan-permitted call change, not another MoltenVK configuration toggle.

Experiment 0015's source candidate now selects a distinct
`reset-no-pipeline-cache` mode. Before the bounded loaded-world reset, pipeline
creation forwards ESO's cache unchanged. During the reset window only, a
non-null cache argument is forwarded as `VK_NULL_HANDLE` and logged. The audit
will fail unless bypassed pipeline totals equal every reset graphics pipeline
and its cache-use counter is zero. Neither on-disk cache is modified by this
code path.

Source commit `cf0261f` passes 65 Python tests, Python compilation, shell
syntax, whitespace, warnings-as-errors, Clang static analysis, the full
pristine-loader build, reset/render-audit probes, and all seven effective
configuration probes. The reset probe proves both halves of the scope: a
startup pipeline retains its non-null cache, while an active reset pipeline
receives null and emits the bypass record. Fresh replacement and embedded
probes both reached the Apple M4.

Experiment 0015 is installed in exact `reset-no-pipeline-cache` mode. Built and
installed hashes match, the target and marker are current, and the quick gate
is `READY`. The warmed active cache, old backup, and settings file were
preserved byte-for-byte. Fresh ignored evidence is prepared under
`artifacts/experiment-0015-20260725T131534Z`. The next gate targets the
recovered 60 FPS state: if the known 8 FPS state appears first, exit Steam
without changing a setting and relaunch once; at ordinary performance, make
one 2048 x 1280 to 1920 x 1200 resolution change and stop after classifying the
output.

The user completed that gate in the recovered 60 FPS state, and persistent
solid-color output recurred after the single resolution change. The bounded
audit proved complete cache exclusion: all 150 reset-created graphics
pipelines were forwarded with `VK_NULL_HANDLE`, zero used a cache, and all
1,648 observed binds of those pipelines matched their render pass. Eight
submitted command buffers still completed 484 render passes and 9,655 indexed
draws without Vulkan failure, state overflow, stale known descriptor rebound,
live image overlap, tracked layout mismatch, dead attachment, or crash.
Pipeline-cache corruption is therefore excluded and cache experiments end.

The leading unresolved scope is descriptor state created before the current
mirror activates. Many sampled final-pass set-index-0 binds are live but have
`last_update_sequence=0`; the mirror began only at the first successful
swapchain, so their slot contents are unknown rather than proven clean. The
next gate is source-only instrumentation that begins descriptor mirroring at
process startup and joins known/unknown slot coverage to ESO's own command
buffer reset, record, bind, and submit generations. No user launch is required
until that instrumentation and its non-game checks are complete.

Experiment 0016 now implements that source-only gate as one integrated
`full-lifetime-audit` candidate. Descriptor layout and slot contents begin at
process sequence 1 and include image views, samplers, buffers, buffer views,
immutable samplers, copies, frees, and pool resets. The same bounded window
records ESO command-buffer generations; render-pass load/store/clear and
attachment roles; aspect/mip/layer transitions with stage/access masks applied
in queue-submit order; and acquire/submit/present semaphore plus fence order.
Fixed-table overflow and unknown coverage are analyzer failures.

That gate is now complete from source commit `7db8803`. All 48 Experiment 0015
evidence checksums were reverified, and a cache-preserving restore produced the
exact original-loader baseline. The full build, 68 Python tests, warnings as
errors, Clang static analysis, all focused probes, all eight effective
configuration probes, and fresh replacement/embedded Apple M4 probes passed.

Experiment 0016 is installed in exact `full-lifetime-audit` mode. Built and
installed proxy and MoltenVK files are byte-identical, the target and marker
are current, the quick gate is `READY`, and both pipeline caches retain their
pre-restore hashes and sizes. The installation record is committed and the
authoritative ignored collection directory is
`artifacts/experiment-0016-20260725T135200Z`. All agent-controlled pre-user
gates are complete. No agent launched Steam, the launcher, or ESO.

The user subsequently reported four attempts under Experiment 0016. The first
two ran at approximately 8 FPS, the third recovered only to approximately
20 FPS, and the fourth declined from approximately 30 to 14 to 8 FPS. Graphics
option changes did not produce solid-color or frozen output in those states.
The raw bridge log contains five ESO process identities, including two without
a completed reset summary; the record does not infer the reason for that
process-count difference.

Three reset windows did complete. Across them, 105,356--116,091 bound
descriptor slots were known and zero were unknown or stale. Command-buffer
generation, descriptor record/update order, queue semaphores and fences,
attachment load/store state, subresource barriers, tracked layouts, and image
memory overlap checks reported no inconsistency. Each window had
16,816--17,888 accumulated mirror overflows, however, so the analyzer correctly
rejected all three as incomplete. Experiment 0016 is inconclusive rather than
a clean public-state pass.

The audit implementation itself is now the leading explanation for the
performance collapse: it retains dead full-lifetime entries, performs several
full fixed-array handle searches, and scans all 131,072 descriptor slots on
resource destruction under one mutex. Its lightly populated smoke benchmark
did not model this load. Do not run the installed `full-lifetime-audit` again.
The ignored evidence directory has 48 checksum-verified files, the startup
verdict passed, no crash report was added, and the exact target remains
current.

The next repair gate is below the observed Vulkan state. MoltenVK 1.4.2 includes
an upstream correction for a cached Metal texture view that can remain attached
to an earlier dynamically replaced swapchain drawable. That mechanism matches
the established condition in which Vulkan acquire, work submission, and
presentation continue while visible output is stale. Experiment 0017 will keep
the 1.4.1 baseline and backport only this correction. A non-game
drawable-replacement probe must distinguish official 1.4.1 from the patched
build before any new game-bundle installation or final user validation.

That differential gate now passes on the Apple M4. A 64 x 64 non-game
swapchain probe rendered changing clear colors through a component-swizzled
image view and read the current drawable's base Metal texture. With the
official release MoltenVK 1.4.1 binary, the same `VkImage` received a different
`MTLTexture` at frame 3, but the new texture remained `0,0,0,0` instead of the
expected current-frame value. The old cached image view had received the
rendering. The probe returned `STALE`.

The exact v1.4.1 source tag with only upstream commit `9a5e233` backported saw
the same base-texture replacement and wrote the exact expected pixel
`191,64,82,255`; every later frame also matched and the probe returned `PASS`.
This confirms the internal failure mechanism without ESO: public Vulkan work
can remain valid while an image view writes an earlier `CAMetalDrawable`.

Experiment 0017 is therefore a narrow repair candidate, not a 1.4.2 upgrade.
Its reproducible source build pins the v1.4.1 source commit, the exact upstream
fix commit, the two-file patch hash, and the v1.4.1 `db445ff` revision embedded
in MoltenVK's pipeline-cache UUID. A distinct `texture-cache-fix` bridge mode
retains descriptor compatibility and disables the Experiment 0016 full-lifetime
audit. The game bundle is still unchanged; the final source and compatibility
gates must pass before requesting new installation approval.
