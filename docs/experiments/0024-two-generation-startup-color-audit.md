# Experiment 0024: two-generation startup color audit redesign

- Date: 2026-08-01
- Outcome: **succeeded; the exact pink startup recorded only submitted opaque-black clears**
- Rollback: **complete; normal `performance-aggressive` marker restored with both caches preserved**

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

## Approved installation checkpoint

The user explicitly approved installation. An initial attempt correctly made
no change because Steam was still running. After the user fully exited Steam,
the update gate again recognized the exact ESO build. The installer then:

1. restored the pristine Bink loader with cache preservation enabled;
2. verified the original loader and absent active marker;
3. installed the exact rebuilt bridge and official MoltenVK 1.4.2;
4. wrote marker `startup-color-audit`;
5. left the active and old-backup caches in place.

Post-install verification at 17:24 +0900 records:

```text
bridge proxy SHA-256:
  766526a899c07523790ac753959ab99a522af5b6e8993e1cadd690094ec8cc71
re-export target SHA-256:
  f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
official MoltenVK 1.4.2 SHA-256:
  aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
active 1.4.2 cache SHA-256:
  8316fda8dae6a03ac2c62cb40986ce0ae3ef08703f9c21d911760b88c096ba45
old-backup cache SHA-256:
  72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
```

Installed bridge, re-export target, and runtime match their build artifacts
byte-for-byte. The pristine restore source matches the independently preserved
ESO copy. Cache hashes are identical before and after restore/install. Settings
were not changed.

## User-run gate

One normal Steam-path startup is now justified. Stop at account login or after
30 seconds, whichever comes first, then quit ESO and Steam normally. Stop
earlier for a crash, hang, or unexpected corruption. No gameplay or additional
screenshot is required.

Exact magenta in a submitted generation-1 or generation-2 clear confirms the
clear source. Only black/non-magenta or no submitted clear rejects that
hypothesis and leaves the known FX-material and window/layer candidates.
Missing two-generation submit/present linkage is inconclusive. After evidence
collection, restore the normal `performance-aggressive` marker and preserve
both caches.

## User-run result

The user launched ESO through the normal Steam path at approximately 17:30:45
+0900 and directly observed the same full-screen pink startup frame. The exact
bridge run is `20260801T083045.794452000Z-pid65194`. The bridge reported the
intended `startup-color-audit` mode and the exact effective aggressive
configuration:

```text
live_resources=0 metal_argument_buffers=0 use_mtlheap=1
synchronous_queue_submits=0 command_pooling=1 prefill=0
maximize_concurrent_compilation=1
```

The audit passed its linkage checks and finished once, exactly at generation-2
present ordinal 180. It recorded no detail-limit event and emitted no bounded
startup/lifecycle record after the finish gate. The linked color operations
were:

```text
generation 1: 1 submitted vkCmdClearAttachments, rgba 0,0,0,1
generation 2: 180 submitted vkCmdClearAttachments, rgba 0,0,0,1
render-pass LOAD_OP_CLEAR values: none
```

All 181 clears covered their complete generation extent. Generation 1 used
3420 x 2148 and generation 2 used 3420 x 2146. The analyzer verdict is `PASS`.

This falsifies the submitted-clear hypothesis for the exact run in which the
user saw canonical magenta: neither ESO's swapchain-linked
`vkCmdClearAttachments` values nor a render-pass load clear supplied magenta.
It does not by itself prove which later draw supplied the pixels.

Follow-up static inspection further weakens the window/layer-background
alternative. ESO's concrete `ZOMetalGameView` returns `YES` from `isOpaque`,
creates a `CAMetalLayer` as its backing layer, and its `drawRect:` obtains
`NSColor.blackColor` and fills the complete bounds with `NSRectFill`. The
executable has no `setBackgroundColor:` selector. Together with the exact
magenta FX-material initializer and the availability of pipeline/draw entry
points in generation 2, the application-owned FX-material/draw path is now the
leading source. This remains a strongly supported inference until a presented
draw or pixel is joined to that material path.

For the fingerprinted executable, the relevant static method implementations
are `makeBackingLayer` at `0x1035fb764`, `isOpaque` at `0x1035fb8ab`, and
`drawRect:` at `0x1035fb8dd`. The latter resolves the `blackColor` and `set`
selectors, obtains the view bounds, and calls the `_NSRectFill` stub. These are
static file addresses, not a live-process trace.

ESO had exited when post-run analysis began. The only Steam-named process was
an orphaned `ipcserver` with parent PID 1; `steam_osx`, ESO, and the launcher
were absent, and `lsof` showed that the helper held no ESO bundle file open.
The normal installation safety gate was therefore satisfied.

At approximately 17:39 +0900, a cache-preserving restore returned the active
loader to the pristine Bink and displaced the diagnostic marker. A subsequent
cache-preserving install restored the `performance-aggressive` marker. Final
verification reports the recognized ESO target, current official MoltenVK
1.4.2, current bridge target, and installed proxy SHA-256
`766526a899c07523790ac753959ab99a522af5b6e8993e1cadd690094ec8cc71`,
which matches the validated build byte-for-byte.

ESO updated the active 1.4.2 cache during the user run while retaining its
7,977,079-byte size. Its pre-rollback and post-reinstall SHA-256 are both
`498afb3db97c57c6fe6b0baef5307bf0c6a9330a73478519caa6cf659474a55b`.
The old-backup cache remained 6,800,792 bytes with SHA-256
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.
No cache was deleted, renamed, or substituted by rollback.

The game itself rewrote `UserSettings.txt` at 17:31, before rollback. Its full
post-run hash is
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`,
while every one of the 48 allowlisted standard graphics/display/performance
values is unchanged. The restore and installer do not write that file.
