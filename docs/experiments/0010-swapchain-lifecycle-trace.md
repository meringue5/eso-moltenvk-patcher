# Experiment 0010: swapchain lifecycle trace

- Date: 2026-07-25
- Outcome: **planned; source candidate statically verified**
- Rollback: **not started; Experiment 0009 bridge remains installed**

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
- full bridge rebuild: pending the technically required restore of the active
  proxy to the pristine loader
- installation: pending explicit approval

The Experiment 0009 pipeline-cache state will be preserved through the
restore/reinstall boundary. No game, Steam, or launcher process may be running
for that operation.

## Procedure

After the source candidate is committed and explicit installation approval is
received:

1. confirm ESO, Steam, and the launcher are stopped;
2. preserve both pipeline caches and restore only the pristine Bink loader;
3. fetch the pinned MoltenVK release and rebuild from source;
4. run every non-game smoke and compatibility probe;
5. prepare fresh ignored evidence;
6. install the rebuilt descriptor-compatibility bridge while preserving both
   pipeline caches;
7. re-run status and artifact fingerprint checks.

Only then ask the user for one Steam-authenticated cold-start run:

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

Pending installation and the bounded user-controlled run.

## Interpretation

Pending.

## Rollback

Not started. The currently installed Experiment 0009 bridge remains the active
loader until installation approval permits the cache-preserving technical
restore and rebuild.

## Follow-up

If the cold start passes, use the trace to design one separately approved,
single-variable live-reset comparison. Do not ask for a live graphics change
until the cold-start evidence and generation summary have been collected.
