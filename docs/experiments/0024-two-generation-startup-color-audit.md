# Experiment 0024: two-generation startup color audit redesign

- Date: 2026-08-01
- Outcome: **two-generation non-game validation passed; candidate not installed**
- Rollback: **not required; game bundle, runtime, profile, caches, and settings were unchanged**

## Question

Can Experiment 0023's color-command discrimination be extended across the
corrected swapchain without entering an unbounded per-frame diagnostic path?

## Trigger

The Experiment 0022 screenshot amendment places exact sRGB `#FF00FF` roughly
2.645 seconds after generation 2 was ready. That falsifies Experiment 0023's
generation-1-only stopping rule before installation. The logging mechanism is
still useful, but a finish at generation-2 creation would discard the required
evidence.

## Candidate boundary

The redesigned `startup-color-audit` retains the exact effective
`performance-aggressive` MoltenVK configuration. It tracks swapchain-linked
framebuffers and submitted command buffers for generations 1 and 2, including:

- render-pass area and `LOAD_OP_CLEAR` color values;
- `vkCmdClearAttachments` RGBA, aspect, attachment, and rectangle values;
- command-buffer/framebuffer association at `vkQueueSubmit`;
- the first eight ordinary lifecycle acquire/present records for each generation.

The audit finishes after generation 2 reaches present ordinal 180. Independently,
all startup-color detail records share a hard limit of 2,048. After the finish
flag, every wrapper directly forwards to MoltenVK without tracking or logging.
The present bound covers about three seconds at 60 FPS while remaining finite;
the record cap bounds pathological multi-pass behavior before that point.

`tools/analyze_startup_color_audit.py` now requires successful same-queue
submit-before-present linkage for both generations and reports each linked RGBA
with its generation. A finish or detail-limit record alone is not interpreted
as a color result.

An exact-magenta submitted clear would decide the immediate source. A black or
absent clear would falsify only the clear hypothesis: static analysis now shows
that ESO uses exact magenta as an FX-material parameter default, so such a
result must not be interpreted as proof of window/layer background exposure.

## Validation performed

No Steam, launcher, or ESO process was started or controlled. The AppKit/Metal
probe was deferred while the user was playing and run only after the bridge log
showed normal device, surface, and instance teardown. No installed file was
changed.

The CPU-only lifecycle probe exercises explicit magenta, opaque black, and
load-only generation-1 paths; creates and clears a generation-2 framebuffer;
links both generations to submits/presents; reaches exactly ordinal 180; and
proves that later wrapper calls produce no audit output. It passes with strict
compiler warnings.

The first unrestricted real-MoltenVK run rendered and recorded the expected
pixels, but exposed two validation defects before completing the suite:

- the load-only assertion counted the intentional generation-2 black clear as
  though it belonged to generation 1;
- the shared lifecycle rule logged all 180 `VK_SUBOPTIMAL_KHR` presents rather
  than respecting the audit's small present-record boundary.

The assertion now matches generation 1 explicitly. Startup-audit
acquire/present records, including nonfatal `VK_SUBOPTIMAL_KHR`, stop after
ordinal 8 while the silent counter still reaches the ordinal-180 finish. The
analyzer recognizes both `VK_SUCCESS` and `VK_SUBOPTIMAL_KHR` as presented
results.

The corrected official-MoltenVK 1.4.2 suite passes nine fresh processes:

```text
64x66 -> 64x64
  explicit magenta: BGRA 255,0,255,255 -> black
  explicit black:   BGRA   0,0,0,255 -> black
  load-only x5:     BGRA   0,0,0,0   -> black

3420x2148 -> 3420x2146
  explicit black:   BGRA   0,0,0,255 -> black
  load-only:         BGRA   0,0,0,0   -> black
```

Every process links both generations to submit/present records, emits only the
first eight generation-2 acquire/present records, and finishes exactly at
ordinal 180.
Pixel output and the recorded color operations agree in all controls.

Python discovery passes all 99 tests, including two-generation analysis,
startup-mode checks, and `VK_SUBOPTIMAL_KHR` handling. Python compilation,
shell syntax, and `git diff --check` also pass. A full source build against a
temporary read-only view of the recognized ESO executable, pristine Bink, and
framework passes Bink re-export, Rosetta self-patch, compatibility probes,
both audit probes, and every mode-configuration probe. The installed bundle
was not used as a build output or changed.

The unrelated build-script accommodation that had allowed rebuilding from the
pristine Bink backup while a proxy was active was removed. It is not part of
this diagnostic.

## Result and next gate

The redesigned boundary is bounded and passes synthetic, real-MoltenVK, and
full-build validation. It is sufficiently discriminating for one targeted ESO
startup: exact magenta in a submitted clear decides that source; black or no
clear rejects only the clear hypothesis and leaves the known FX-material and
window/layer candidates.

The candidate remains uninstalled. Modifying the game bundle requires separate
explicit approval, after which the exact installed mode/configuration and
restore path must be checked before one Steam-path startup is requested.
