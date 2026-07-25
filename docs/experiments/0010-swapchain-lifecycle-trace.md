# Experiment 0010: swapchain lifecycle trace

- Date: 2026-07-25
- Outcome: **failed to identify a swapchain-lifecycle fault; live reset still
  corrupts rendered content**
- Rollback: **not performed; restore path rechecked**

## Question

Does the preserved 1920 x 1200 setting render correctly when established by a
cold start, and what exact swapchain-dependent resource sequence occurs before
the first presentations of every swapchain generation?

## Hypothesis

If the Experiment 0009 corruption belongs to ESO's loaded-world live-reset path
rather than the resolution itself, a cold start at the already preserved
1920 x 1200 setting will reach the world with correct rendering. Every
successfully created swapchain generation should acquire and present images,
and the previous generation should have no live image views or framebuffers
when destroyed.

The hypothesis fails if cold-start rendering is already solid-color, a new
generation never presents, acquire/present reports an error, generations are
mixed in one framebuffer, or a swapchain is destroyed while a tracked image
view or framebuffer remains live.

## Target and change set

- ESO client: 12.0.7, databuild `3281538`
- Executable SHA-256:
  `82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`
- MoltenVK: 1.4.1 in the existing descriptor-compatibility configuration
- HDR extension and exact surface-format filters: unchanged
- Redirect set: unchanged at 17 static entry points
- Active settings: 1920 x 1200, ambient occlusion disabled,
  `SkipPregameVideos` enabled

The candidate changes only the function pointers returned for these existing
GDPA queries:

```text
vkDeviceWaitIdle
vkCreateSwapchainKHR
vkDestroySwapchainKHR
vkGetSwapchainImagesKHR
vkCreateImageView
vkDestroyImageView
vkCreateRenderPass
vkDestroyRenderPass
vkCreateFramebuffer
vkDestroyFramebuffer
vkAcquireNextImageKHR
vkQueuePresentKHR
```

Each wrapper forwards the original arguments and result unchanged. It assigns a
monotonic generation to each successful swapchain creation, connects
swapchain images to their image views and framebuffers, records dependent
destruction, and logs the first eight acquire/present calls for each generation
plus every non-success result. It does not alter a Vulkan create structure,
destroy order, result, or presentation request.

## Preflight

- `scripts/quick-update-check.sh`: `READY`
- `scripts/status.sh`: Experiment 0009 bridge installed, current, and enabled
- Python unit tests: 42 passed
- lifecycle forwarding smoke probe: passed
- bridge and lifecycle sources: clean under `-Wall -Wextra -Werror`
- Clang static analyzer: no finding
- full bridge rebuild: passed from source commit `2594b31`
- installation: explicitly approved and completed in descriptor-compatibility
  mode

The Experiment 0009 pipeline-cache state was preserved through the
restore/reinstall boundary. The first restore attempt failed closed because
Steam was still running and changed no file. After Steam stopped, the process
gate passed.

## Installation checkpoint

The cache-preserving technical restore returned only the Bink loader to the
pristine file. The pinned MoltenVK 1.4.1 release was reverified, and the full
build passed Bink re-export, Rosetta self-patch, HDR filter, lifecycle trace,
MoltenVK configuration, legacy/current Vulkan proc, and legacy/current
surface-format probes.

The rebuilt files were then installed with both pipeline caches left in place.
Installed and built hashes match:

```text
libBink2Macx64.dylib
3f3f8dce7a400140f14edb0595e0892945df297d6b8a870c5f4f77932b51101a

libMoltenVK.teso4m4.dylib
d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398
```

Post-install status reports the selected ESO target current, the bridge
installed and current, and the enable marker present. The quick gate is
`READY`. The ignored evidence boundary is
`experiment-0010-20260725T103330Z`.

The exact active settings hash is
`5912a842e7157a73d189f43a1faaf6d8a7635b7cae8a6c0920afff417ae6b21a`,
with 1920 x 1200, ambient occlusion `0`, and `SkipPregameVideos` `1`.
The active 4,041,740-byte pipeline cache remains
`229fc841f078176f845e3654690d4ab1f478231d9659150096ef45e572c516c7`;
the 6,800,792-byte old backup remains
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.

## Procedure

The following installation steps are complete:

1. confirm ESO, Steam, and the launcher are stopped;
2. preserve both pipeline caches and restore only the pristine Bink loader;
3. fetch the pinned MoltenVK release and rebuild from source;
4. run every non-game smoke and compatibility probe;
5. prepare fresh ignored evidence;
6. install the rebuilt descriptor-compatibility bridge while preserving both
   pipeline caches;
7. re-run status and artifact fingerprint checks.

The remaining step is one user-controlled Steam-authenticated cold-start run:

- do not open the graphics menu or change a setting;
- enter the existing character and reach the world;
- remain for at most three minutes;
- stop immediately on persistent solid-color output, flicker, or a crash;
- otherwise exit normally after the bounded interval.

No Metal HUD, screenshot, manual FPS capture, or exploratory travel is needed.
The run answers rendering correctness and lifecycle classification only.

## Evidence

Evidence preparation preserves the exact before state. Collection now invokes
`tools/analyze_lifecycle_log.py`, which selects only a run after the preparation
time and summarizes each swapchain generation, resource create/destroy counts,
first acquires/presents, and lifecycle anomalies.

## Result

Installation and evidence preparation passed. The bounded user-controlled run
is pending.

## Interpretation

Pending.

## Rollback

Not performed. The Experiment 0010 bridge is installed, and the pristine loader
plus both pre-existing pipeline caches remain available.

## Follow-up

If the cold start passes, use the trace to design one separately approved,
single-variable live-reset comparison. Do not ask for a live graphics change
until the cold-start evidence and generation summary have been collected.

## Run and recovery amendment: 2026-07-25

The user completed multiple Steam-authenticated runs after installation. A
cold start reached the world and rendered normally. One first run briefly
showed approximately 8 FPS and an ESO cursor-mode symptom, but the next launch
did not reproduce either. WindowServer records identify ESO as frontmost and
receiving keyboard focus, so OS background throttling does not explain that
one run.

The controlled live graphics reset reproduced the visual failure. The third
swapchain generation was created at 3420 x 2146, acquired and presented 396
frames, and reported `VK_SUBOPTIMAL_KHR` throughout. All tracked swapchain
images, views, render passes, and framebuffers followed a clean lifecycle.
No mixed-generation framebuffer, live dependent at swapchain destruction,
device loss, or lifecycle capacity error was recorded.

Three recovery probes were then tested separately and rejected:

1. Promoting the first generation-3 present result from suboptimal to
   out-of-date stopped subsequent presentation; ESO kept acquiring and did not
   recreate the swapchain.
2. Promoting the first generation-3 acquisition result to out-of-date likewise
   produced no recreation. The last scene remained frozen while audio and
   input continued.
3. Adding compatible stretch presentation scaling made generation-3
   acquisition and presentation return success instead of suboptimal, but the
   user still saw solid-color output.

These probes establish that the persistent `VK_SUBOPTIMAL_KHR` result was a
symptom, not the cause of the corrupted output. The final image was already
invalid before successful presentation. Each result-changing probe was removed,
the observation-only Experiment 0010 source was rebuilt, and that checkpoint
was reinstalled. The combined raw bridge log is checksum-preserved in the
ignored `artifacts/experiment-0010-postmortem-20260725T112500Z` directory.

The remaining boundary is a non-swapchain resource or render-state failure
during loaded-world recreation. MTLHeap allocation is the first single
variable because it is active in MoltenVK 1.4.1 on Apple GPUs, absent from the
embedded 1.0.18 runtime, and directly changes resource placement and reuse.
