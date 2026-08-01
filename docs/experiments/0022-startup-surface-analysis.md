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

## 2026-08-01 screenshot amendment

A user-controlled ordinary startup supplied the missing pixel capture after
the original analysis was written. The agent did not launch or control ESO and
did not change its runtime, profile, caches, settings, or bundle. The PNG has
SHA-256
`e53278006d1d12201b71e7ea6a3833964aaa6fab9f0a9d3022ca65fa4170777d`,
dimensions 3420 x 2214, and an embedded Display P3 profile. Its dominant stored
P3 value is `(234,51,247)` over 92.38% of all pixels. ICC conversion to sRGB
maps that value exactly to `(255,0,255)`, or `#FF00FF`; this is a measured
canonical magenta frame rather than a subjective hot/neon-pink description.

The verified ESO executable contains 24 exact 16-byte float sequences for
`{1,0,1,1}` in `__TEXT,__const`; the official MoltenVK 1.4.2 dylib and installed
bridge contain none. Static instruction decoding finds seven real code
references to one pooled ESO copy. Several references initialize the same
parameter structure from two code paths that subsequently resolve
`technique_FXMaterial` or `technique_FXMaterialTransparent`. This makes an ESO
FX-material default/error color a locally supported candidate rather than a
generic Unity analogy. No static call chain yet connects that structure to the
startup surface, so it does not prove that an FX material produces this frame.
It does support application-owned canonical magenta over a MoltenVK hard-coded
sentinel.

For the verified executable, the pooled constant is at VA `0x1039019f0`.
Initializer `0x1035fcd42` loads it at `0x1035fcd5b`; direct callers at
`0x1001ba0d7` and `0x1001bb468` later select the literal FX-material technique
names. These are static file addresses, not a live-process trace.

The top 68 rows are black macOS chrome. The remaining content is exactly
3420 x 2146, and rows 284 through 1864 are uniformly the dominant magenta
across the full width. macOS Game Mode and screenshot-thumbnail overlays retain
normal colors over the magenta content. This confines the failure to the ESO
window/layer content and weighs strongly against global HDR, display color
mapping, or an overlay-compositor failure.

The corresponding existing client log records first reset ready at
15:16:51.515 and corrected reset ready at 15:16:52.355. The screenshot's
capture name records 15:16:55, approximately 2.645 seconds after the corrected
surface was ready and 1.503 seconds after the interface log entered
`AccountLogin` at 15:16:53.952. Filesystem creation at 15:17:02 reflects the
later screenshot-save operation and is not used as the capture time.

This evidence supersedes the original claim that the *visible color's
lifetime* was localized to generation 1. The lifecycle fact remains valid:
generation 1 presents once and is replaced after roughly 0.8--0.9 seconds.
The screenshot proves that the magenta content itself persists or is
reproduced after that replacement. A 3420 x 2148 drawable could also be
clipped into the 3420 x 2146 content area, so dimensions alone cannot identify
the presented generation. The next audit must cover generation 1 and bounded
early generation-2 submissions/presents; stopping at generation-2 creation is
not discriminating.

The original missing-shader argument is consequently narrower than first
stated. Proc order still excludes a shader draw as the source of the one
captured generation-1 submission, but shader and pipeline entry points exist
after generation 2. A later full-screen error/FX-material path is not
established, but the exact-magenta FX-material initializer means it must remain
alongside the leading clear/background candidates. The lifecycle evidence
alone cannot exclude it from the complete visible interval.
