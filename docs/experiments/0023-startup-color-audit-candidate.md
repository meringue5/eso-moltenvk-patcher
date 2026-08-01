# Experiment 0023: bounded startup color audit candidate

- Date: 2026-08-01
- Outcome: **non-game mechanism succeeded; generation-1-only scope was invalidated before installation**
- Rollback: **not required; game bundle, installed marker, caches, and settings were unchanged**

## Question

Can a diagnostic wrapper record the exact color operation that reaches ESO's
one-present generation-1 framebuffer, associate it with a successful queue
submission and first present, and stop before generation 2 without changing
the working MoltenVK configuration?

## Hypothesis

If ESO supplies the observed startup color through `vkCmdClearAttachments` or
a render-pass `LOAD_OP_CLEAR`, a generation-1-only audit will record the exact
float RGBA on a command buffer submitted before the first present. If the
submitted frame contains no color clear, the ESO-clear hypothesis is falsified
and application window/CAMetalLayer background exposure becomes the leading
candidate.

## Constraints and state

The agent did not start Steam, the launcher, or ESO and did not install the
candidate. The installed marker remains `performance-aggressive`; official
MoltenVK 1.4.2, active and preserved pipeline caches, and user settings were
not changed. The target gate still recognizes ESO 12.0.7 databuild `3281538`,
SHA-256
`82bc04ebc8c486636303d147edb9af6c0727b19c7faf7ce7d00837ac3e8ebf4d`,
and UUID `6599A49F-B1A0-3CBF-9AEF-6D4186A66E0D`.

## Candidate design

The `startup-color-audit` marker mode retains the exact effective
`performance-aggressive` MoltenVK configuration:

```text
live_resources=0
metal_argument_buffers=0
use_mtlheap=1
synchronous_queue_submits=0
command_pooling=1
prefill=0
maximize_concurrent_compilation=1
```

It enables only the already bounded lifecycle wrapper plus these records for
swapchain generation 1:

- framebuffer and render-area identity at `vkCmdBeginRenderPass`;
- color attachment values whose render-pass load operation is `CLEAR`;
- every `vkCmdClearAttachments` float RGBA, aspect, color attachment, and
  clear rectangle;
- the generation-1 command buffer and framebuffer passed to a successful
  `vkQueueSubmit`;
- the existing first-present record, including queue and result.

The second successful swapchain creation emits
`STARTUP_COLOR_AUDIT_FINISH` and suppresses all later audit/lifecycle output.
The Vulkan calls and argument values are forwarded unchanged. Fixed-capacity
tracking and per-call record limits prevent unbounded collection.

`tools/analyze_startup_color_audit.py` requires an exact run ID, checks that a
generation-1 submit precedes a successful present on the same queue, associates
the submitted framebuffer with its color records, and classifies the result as
`explicit-clear-attachments`, `render-pass-loadop-clear`, or
`no-submitted-color-clear`.

## Non-game evidence

The lifecycle smoke probe passed three synthetic command sequences at
3420 x 2148 and proved that logging ends at generation 2:

```text
neon pink: rgba=1,0,1,1
black:     rgba=0,0,0,1
load-only: no STARTUP_COLOR_CLEAR record
```

The official MoltenVK 1.4.2 AppKit probe then coupled those audit wrappers to
real rendering and Metal pixel readback. It passed the same three cases, five
fresh load-only processes, and the exact 3420 x 2148 to 3420 x 2146 surface
replacement. In each case the logged command buffer and framebuffer appeared
in `STARTUP_COLOR_SUBMIT` before `SWAPCHAIN_PRESENT` on the same queue. The
pixel and audit classifications agreed:

```text
neon pink: BGRA 255,0,255,255; audit rgba=1,0,1,1
black:     BGRA   0,0,0,255; audit rgba=0,0,0,1
load-only: BGRA   0,0,0,0;   no submitted color clear
```

The exact diagnostic mode configuration probe passed. A clean source rebuild
passed Bink re-export, Rosetta self-patch, compatibility probes, all mode
configuration probes, and the expanded lifecycle smoke test. Python discovery
ran 98 tests successfully; Python compilation, shell syntax, and diff checks
also passed.

## Result

The mechanism discriminates the three non-game inputs, but the candidate is
not sufficiently scoped for an ESO startup and remains uninstalled. The later
pixel capture described below shows that stopping at generation-2 creation can
miss the visible magenta frame. Existing logs do not contain ESO's real clear
arguments, and a non-game client cannot generate ESO's command buffer.

## Post-candidate screenshot evidence

A user-controlled ordinary startup produced a 3420 x 2214 Display P3 PNG whose
ESO content area is 3420 x 2146 and whose dominant color converts exactly to
sRGB `#FF00FF`. The capture time is approximately 2.645 seconds after the
client log says the corrected generation-2 surface was ready. Normal-colored
macOS overlays are composited over the magenta ESO content.

That observation invalidates the candidate's generation-1-only stopping rule
before it was installed. It does not invalidate the logging mechanism or its
non-game discrimination results. The candidate must be redesigned to retain
bounded submit/present linkage through early generation 2 and then revalidated
without ESO.

## Cancelled user-run gate

Do not install or request a user run with this candidate. Its stopping boundary
is now known to be insufficient. Any replacement still requires explicit
approval for reversible game-bundle installation and must preserve both caches,
user settings, and the exact effective performance profile above.

The former pass criteria of one generation-1 submit/present followed by a
generation-2 finish are withdrawn. Replacement criteria must link color
operations to generation 1 and a small, fixed number of early generation-2
successful presents, then stop without entering an unbounded per-frame path.
