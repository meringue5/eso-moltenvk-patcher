# Experiment 0022: transient startup surface analysis

- Date: 2026-08-01
- Outcome: **inconclusive on the final pixel source; transient surface boundary localized**
- Rollback: **not required; installed runtime, profile, caches, settings, and game bundle were unchanged**

## Question

What produces the full-screen neon-pink/hot-pink frame visible for roughly one
second at ESO startup, and which existing evidence can distinguish it from a
missing asset, pregame video, display-setting, or MoltenVK default-clear issue?

## Constraints

The installed official MoltenVK 1.4.2 `performance-aggressive` checkpoint,
active and preserved pipeline caches, and user settings remained unchanged.
The agent did not start Steam, the launcher, or ESO. Analysis used the verified
ESO 12.0.7 executable, existing run logs, static disassembly, and a standalone
MoltenVK/AppKit probe.

The initial gate passed for ESO databuild `3281538`, executable SHA-256
`82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`,
UUID `6599A49F-B1A0-3CBF-9AEF-6D4186A66E0D`, and official MoltenVK 1.4.2
SHA-256
`aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.

## Existing-log correlation

`tools/analyze_startup_surface.py` filters an exact bridge run ID, checks the
first two swapchain generations, and correlates them with ESO's first two
device-reset timestamps. It rejects a lifecycle unless the second swapchain
replaces the first, changes only the height by two pixels, and the first
generation has exactly one successful acquire and present.

Two independent lifecycle traces pass that classification:

```text
run 20260725T103547.901947000Z-pid25514
  generation 1: 3420x2148, old generation 0
  generation 1: one successful acquire, one successful present
  generation 2: 3420x2146, old generation 1
  first reset ready -> correction starts: 852 ms
  first reset ready -> corrected reset ready: 854 ms
  corrected reset ready -> AccountLogin: 1587 ms

run 20260725T130615.206232000Z-pid43186
  generation 1: 3420x2148, old generation 0
  generation 1: one successful acquire, one successful present
  generation 2: 3420x2146, old generation 1
  first reset ready -> correction starts: 796 ms
  first reset ready -> corrected reset ready: 802 ms
  corrected reset ready -> AccountLogin: 1565 ms
```

The latest ordinary MoltenVK 1.4.2 client log has no lifecycle wrapper by
design, but repeats the timing shape: first reset ready to correction is
908 ms and first reset ready to corrected reset ready is 910 ms.

This interval matches the user's approximate one-second observation. The
corrected surface is ready well before the first logged `AccountLogin` state.
Experiment 0008 independently established that skipping every logged pregame
video/logo state does not remove the frame.

The sanitized settings template contains logical fullscreen dimensions
2048 x 1280, not either 3420-pixel backing extent. The observed two-pixel
transition is therefore a macOS surface/backing-size convergence event, not
evidence that the user typed 3420 x 2148 into `UserSettings.txt`.

## Static render-path analysis

The exact executable constructs its Vulkan color attachment descriptions with
`VK_ATTACHMENT_LOAD_OP_LOAD`, `VK_ATTACHMENT_STORE_OP_STORE`, and general
initial/final layouts. It performs explicit clearing separately through a
full-render-target `vkCmdClearAttachments` path. The clear command copies a
dynamic four-float color supplied by its caller; the leaf implementation does
not contain a fixed neon-pink sentinel.

The first-generation proc trace obtains `vkCmdBeginRenderPass` and
`vkCmdClearAttachments` before the first acquire/submit/present. It does not
obtain `vkCreateShaderModule`, `vkCreateGraphicsPipelines`,
`vkCmdBindPipeline`, or `vkCmdDrawIndexed` until after generation 2 has already
presented. This makes a missing texture/material shader an implausible
explanation for the generation-1 full-screen frame: the captured early path is
clear-only or load-only, not a shader draw.

Several statically visible early clear callers supply opaque black
`{0, 0, 0, 1}`. Other clear callers pass dynamic values, so static analysis
does not establish the exact value used by the one startup submission.

## Non-game MoltenVK probe

`scripts/probe-startup-surface.sh` builds an x86_64 AppKit/Vulkan probe against
official MoltenVK 1.4.2. It uses the exact installed aggressive configuration,
creates a first surface two pixels taller than its replacement, submits one
render pass with no shader or draw, reads the current Metal drawable, presents
it, then replaces the swapchain and explicitly clears the corrected surface to
black.

The explicit neon-pink control produced the expected BGRA bytes, the black
control stayed black, and five fresh load-only processes all returned
transparent black. The probe treats a load-only neon-pink result as a failure,
while recognizing that Vulkan does not define preserved content for a fresh
swapchain image:

```text
neon-pink 64x66: 255,0,255,255 -> corrected black 0,0,0,255
black      64x66:   0,0,0,255 -> corrected black 0,0,0,255
load-only  64x66:   0,0,0,0   -> corrected black 0,0,0,255 (5/5)
```

The exact ESO backing extents behave the same way:

```text
black     3420x2148: 0,0,0,255 -> 3420x2146: 0,0,0,255
load-only 3420x2148: 0,0,0,0   -> 3420x2146: 0,0,0,255
```

The probe requires normal Metal/IOSurface access and was run outside the
restricted command sandbox. It did not load or modify any ESO component.

## Result

### Confirmed

- The visible-duration boundary is the first, transient 3420 x 2148 backing
  surface: it presents exactly once and is replaced by 3420 x 2146 after
  796--908 ms in the three available timestamped runs.
- The transition finishes before the logged account-login UI state.
- The first captured render path precedes shader creation, pipeline binding,
  and drawing; missing game textures or shaders do not explain this frame.
- In the tested official MoltenVK 1.4.2 configuration, no uncleared first
  drawable initialized to neon pink in five small fresh-process trials or at
  ESO's exact backing extent. Fresh load-only contents are not guaranteed by
  Vulkan, so this is an implementation observation rather than an API promise.
- A normal opaque-black explicit clear remains black at 3420 x 2148; the
  two-pixel mismatch alone does not transform it into pink.

### Inference

ESO exposes a temporary, not-yet-converged macOS surface for approximately one
second. The neon-pink/hot-pink content most likely comes from a dynamic
full-surface clear recorded by ESO during that interval. Exposure of an
application window/layer background remains a secondary possibility.

### Not established

The existing traces record proc lookup and lifecycle, not the actual
`VkClearAttachment.clearValue` or drawable pixel for generation 1. Static
analysis also finds dynamic clear callers. The exact neon-pink value and
whether it is written by `vkCmdClearAttachments` therefore remain unproven.

## Interpretation of generic startup advice

- `SkipPregameVideos=1` is already a failed controlled hypothesis.
- Windows overlay hooks and console HDR do not match this macOS Vulkan path.
- A native-resolution edit is not justified: the relevant change is a
  two-pixel backing-surface convergence, while the current logical setting is
  different from both physical extents.
- File repair remains a generic integrity action, not an evidence-backed
  candidate. The current executable fingerprint is exact and the same
  transient repeats across multiple bridge/runtime checkpoints.
- Unity-style magenta missing-shader guidance does not fit a frame that occurs
  before the traced shader/pipeline/draw path.

The color is recorded as `neon pink / hot pink` until a pixel capture proves a
more exact colorimetric name.

## Follow-up

Do not change settings, caches, or the performance profile and do not request
an exploratory user run. The next candidate must be a bounded, startup-only
audit that associates generation-1 framebuffers with
`vkCmdClearAttachments`, records only the color/aspect/rect and begin-pass
clear values needed for the first successful present, and disables itself at
generation 2. It must first pass a synthetic/non-game probe proving that it
distinguishes explicit neon-pink clear, opaque-black clear, and load-only
submission without changing the forwarded Vulkan calls.
