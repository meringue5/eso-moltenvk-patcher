# Project status

Last updated: 2026-08-01

## Current production baseline

`teso4m4` was promoted to production on 2026-08-01. The validated gameplay
baseline is official MoltenVK 1.4.2 with the `performance-aggressive` profile
for ESO 12.0.7, databuild `3281538`.
Experiment 0027 is complete. Experiment 0028's narrower draw-input candidate
has passed its static, synthetic, real MoltenVK/AppKit, analyzer, and clean
build gates. The original loader is temporarily restored for the required
source rebuild; the approved `startup-input-audit` installation is the next
operation. The update check, executable fingerprint, official runtime hash,
and 1.4.2 pipeline-cache UUID remain current.

The user's post-install ordinary play now supplies the missing runtime
validation. The latest preserved run,
`20260731T114051.034860000Z-pid30867`, passed the automatic bridge-startup
verdict. Its client log contains six complete graphics-device reset sequences,
zero reset error markers, repeated loaded-world completions, and no subsequent
ESO crash report. The user reports that resolution and graphics-setting
changes now return to correct scene rendering and that relatively high settings
remain comfortably playable. With the committed 2048 x 1280 standard profile,
the user observed the on-screen counter remain at the 60 FPS VSync ceiling
throughout active gameplay in the roughly 93-minute session, without the
embedded-runtime baseline's recurring sustained drop toward 30--33 FPS.

The preserved ESO, interface, and bridge logs do not contain continuous FPS
samples. They independently establish the session boundary, bridge
configuration, world loads, reset sequences, and absence of a subsequent crash
report; the 60 FPS result is a direct user observation. It applies to the
combined runtime, profile, settings, and cache checkpoint on the tested M4, not
as a guarantee for another machine or an attribution to one MoltenVK change.

This establishes the current 1.4.2 baseline as a successful production gameplay build
on the tested Apple M4 MacBook Air. It does not isolate which 1.4.2 change,
fresh 1.4.2 cache state, or interaction with the existing performance profile
removed the prior reset symptom. The 1.4.1 failures remain valid historical
evidence, but another dedicated reset reproduction is not warranted unless the
problem recurs.

The remaining visible maintenance defect is a transient full-screen canonical-magenta
frame during early startup. A user screenshot with embedded Display P3 maps its
dominant content exactly to sRGB `(255,0,255)` / `#FF00FF`. Its ESO content is
3420 x 2146, while normally colored macOS overlays remain visible above it;
this is window/layer content rather than global display corruption.

Experiment 0022 originally associated the user's approximate duration with the
first 3420 x 2148 backing surface. The lifecycle fact remains: two independent
traces show exactly one successful generation-1 present before a 3420 x 2146
replacement after 796 and 852 ms, and another run repeats the reset timing at
908 ms. The screenshot amendment falsifies the stronger lifetime conclusion.
Its capture time is approximately 2.645 seconds after the corrected surface
was ready and 1.503 seconds after `AccountLogin`, so magenta persists or is
reproduced beyond generation-1 replacement.

Static analysis and proc order place this first frame before ESO's captured
shader/pipeline/draw path. A non-game 1.4.2 probe also shows that load-only
drawables start transparent black and opaque-black clears remain black at the
exact ESO extents. Pregame videos, global HDR/display mapping,
overlay-compositor failure, and a MoltenVK automatic pink default are therefore
excluded as leading explanations. Proc order rules out a shader draw for the
single generation-1 submission, but the screenshot persists into the later
shader-capable interval. Moreover, exact `{1,0,1,1}` is loaded by an ESO
parameter initializer used in code paths that resolve `technique_FXMaterial`
and `technique_FXMaterialTransparent`. An ESO FX-material default/error color
is therefore an application-owned candidate, though it is not yet connected
to startup. Experiment 0024 subsequently excluded the dynamic full-surface
clear, and the opaque view's black fallback weakened window/layer background
exposure. Official MoltenVK and the installed bridge contain no exact
`{1,0,1,1}` float constant. The exact-magenta FX-material or related ESO draw
is now leading, but the exact presented pixel writer remains the active gate.
This is a low-impact startup presentation defect, not a gameplay blocker.

Experiment 0023's logging mechanism correctly distinguishes `{1,0,1,1}`, opaque
black, and load-only submission with real MoltenVK at the exact ESO surface
extents. Its generation-1-only stopping rule is now invalidated before
installation, however: it suppresses logging precisely when the screenshot
shows further evidence is required. The source build and 98 tests pass, but
that candidate was not installed or used for a user run. It was superseded by
the smaller bounded redesign that covers generation 1 and early generation 2.

Experiment 0024 completed that bounded two-generation audit in the exact user
run `20260801T083045.794452000Z-pid65194`. The user observed the same pink
frame, while the audit linked one generation-1 and 180 generation-2 submitted
full-surface clears, all opaque black `(0,0,0,1)`. There was no render-pass
load clear, detail-limit event, or record after the exact ordinal-180 finish;
the analyzer passed. A submitted Vulkan clear is therefore excluded for the
observed frame.

Static inspection also shows that ESO's concrete opaque `ZOMetalGameView`
creates the backing `CAMetalLayer`, while its fallback `drawRect:` fills the
complete bounds with `NSColor.blackColor`; no `setBackgroundColor:` selector is
present. This weakens window/layer background exposure. The known ESO
exact-magenta FX-material default and a generation-2 draw are now the leading
source, but no presented draw has yet been joined to that material, so this is
not yet a confirmed pixel-writer trace.

The apparent remaining Steam process was an orphaned `ipcserver` with parent
PID 1; neither `steam_osx`, ESO, nor the launcher was running, and it held no
ESO bundle file open. A cache-preserving restore and reinstall then returned
the marker to `performance-aggressive`. The installed proxy SHA-256 is
`fc95d3c84d16609d5bd7e300a2753babc7ea262b72dca3681d19cf6c293e1aaf`
and matches the validated build byte-for-byte. Experiment 0025 has likewise
been restored to this normal marker with all caches and settings preserved.

Experiment 0025 now completes that candidate without changing the installed
game. Static analysis narrows the exact initializer at image offset
`0x35fcd42` to two direct callers whose enclosing paths select
`technique_FXMaterial` and `technique_FXMaterialTransparent`. The initializer
writes two `(1,0,1,0)` vectors and one `(1,0,1,1)` vector in a single parameter
block. The new `startup-fx-neutralize` mode runs the original initializer and,
only during Experiment 0024's validated generation-2 ordinal-180 window,
changes those exact matches to black while preserving alpha. Normal
`performance-aggressive` never installs this patch.

The synthetic executable-trampoline probe, exact-byte/constant/caller target
generation, full source build, effective aggressive configuration check, and
104 Python tests pass. The fast update-rebase path now fails closed when an
experimental target is present. The user then explicitly approved installation.
The user completed the one startup and observed unchanged pink. The exact hook
and ordinal-180 finish records are present, but the initializer produced zero
calls inside the bounded window, so no value was neutralized. The analyzer
correctly classifies the intervention as `INCONCLUSIVE`, not as an exclusion.
This establishes that the initializer is not called between hook installation
and the end of early generation 2. Do not repeat this intervention; a successor
must target a later parameter copy/use or presented draw and account for a
possible object created before the bridge constructor. No further user run is
justified until that successor passes its non-game gate.

Experiment 0026 supplied that successor at the final swapchain boundary.
Its isolated `startup-present-pixel-audit` mode reads, but never changes, five
points from twenty scheduled final swapchain images immediately before the real
present. Same-queue semaphore provenance plus a scheduled `vkQueueWaitIdle`
prevents reading unfinished rendering; semaphore waits consumed by intervening
queue submissions are invalidated before later presentation checks. The real
MoltenVK/AppKit controls
distinguish exact RGBA `(255,0,255,255)` from opaque black at both small and
exact ESO extents; the temporary complete bridge link, lifecycle/reset probes,
and all 108 Python tests pass. A complete exact run could therefore choose
between magenta already present in final image content and a post-swapchain
presentation/layer/compositor source.

The user explicitly approved the new mode. Cache-preserving restoration, a
clean source build, and installation completed with Steam, the launcher, and
ESO closed. The installed proxy and MoltenVK match the clean build byte for
byte; active cache, backup cache, and settings hashes remain unchanged. The
first evidence-preparation attempt correctly stopped because the otherwise
matching launcher snapshot was 5,888 seconds old, beyond the fixed 3,600-second
gate. The user refreshed the normal launcher without pressing Play, and the
fresh evidence boundary then passed all eight repository comparisons.

In exact run `20260801T105310.069752000Z-pid86806`, the user observed the pink
frame and all twenty scheduled pre-present summaries completed. Generation 1
ordinal 1 and generation 2 through ordinal 70 were opaque black at all five
points. Generation 2 ordinals 80 through 140 were exact RGBA
`(255,0,255,255)` at all five points. Ordinals 150 through 180 were ordinary
nonblack, nonmagenta scene colors. The analyzer verdict is therefore
`SWAPCHAIN-MAGENTA-CONFIRMED`: ESO/MoltenVK final swapchain content is already
canonical magenta before `vkQueuePresentKHR`. Post-swapchain layer, compositor,
overlay, HDR, and display mapping are excluded as the source.

Experiment 0024 proves the submitted full-surface clear is black throughout
the same bounded interval. Immediately after the last sampled black frame at
ordinal 70, the proc trace first obtains descriptor, graphics-pipeline,
pipeline-bind, vertex/index-bind, and `vkCmdDrawIndexed` entry points; ordinal
80 is the first sampled exact-magenta frame. The active blocker is now to
associate the presented black/magenta/normal frames with their exact draw and
pipeline signatures. No further user run is justified until a bounded
swapchain-linked draw audit passes its non-game gate.

The exact run produced no crash report and left the settings file byte-identical
at SHA-256 `297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.
That Experiment 0026 run updated the active 1.4.2 cache normally to
`8b061e93fa21d2b687ec7eef5bafa363c04418e468928bdaee3a687585773de7`;
the old-backup cache remains unchanged at
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.
The cache-preserving rollback restored `performance-aggressive`; official
MoltenVK and the installed proxy still match their verified build artifacts.

Experiment 0027 completed its active bounded audit and the normal profile is
restored.
Its `startup-draw-audit` mode retains Experiment 0026's exact pixel sampler and
adds submit-semaphore provenance for each sampled frame: draw counts, ordered
draw signature, at most eight stable graphics-pipeline signatures, and separate
vertex/fragment shader hashes. Only command buffers inside a swapchain-linked
render pass contribute. Consumed semaphores, missing identities, capacity
overflow, and incomplete sampling fail closed.

The synthetic draw/submit/present case and fail-closed analyzer controls pass.
A real official MoltenVK 1.4.2/AppKit probe also renders canonical magenta with
one graphics draw at both small and exact ESO extents; the pre-present sample
contains five exact magenta points and the same signal semaphore resolves to
one complete pipeline with stable vertex and fragment hashes. The complete
temporary bridge link, exact aggressive configuration probe, lifecycle smoke,
and all 111 Python tests pass. A cache-preserving restore, clean source build,
and approved install completed with Steam, the launcher, and ESO closed. The
installed proxy and MoltenVK match the clean build byte for byte; both cache
hashes and the settings hash remained unchanged. The fresh evidence boundary
`experiment-0027-20260801T111424Z` began at `2026-08-01T11:14:26Z` from commit
`b29e6dd`.

The user reported visible pink in exact run
`20260801T111610.337976000Z-pid92750`. All twenty scheduled pixel/draw pairs
completed with exact submit-semaphore provenance and no overflow. The nine
opaque-black samples through generation-2 ordinal 70 had zero draws. Every
exact-magenta sample from ordinals 80 through 140 had exactly one indexed draw
using pipeline signature `c43e4410d3b33fe7`, vertex shader hash
`c8307556011c995e`, and fragment shader hash `6907bd3576e3a930`. The analyzer
verdict is `DRAW-PIPELINE-CANDIDATE-ISOLATED`.

The same one draw and pipeline remained present when ordinals 150 through 180
contained normal scene colors. This identifies the sole swapchain draw that
writes the magenta samples, but does not by itself prove a hard-coded shader
color: an input texture, descriptor, uniform/push value, or related resource
can change while the pipeline and draw signature stay fixed. The next gate is
static and non-game descriptor/resource provenance for this exact identity,
not another generic color, clear, or pipeline audit.

No crash report was created and settings remained byte-identical. The active
1.4.2 cache updated normally to
`a9a4ee5112466265c233e2561bb6c284032bfa30f077c0227259f09db682a063`;
the old-backup cache is unchanged. Cache-preserving rollback restored the
`performance-aggressive` marker, and the installed proxy, renamed original,
and official MoltenVK match the verified build byte for byte. No further user
run is requested until a narrower successor passes its non-game gate.

The screenshot run itself started at 15:16:50 and completed normal Vulkan
teardown at approximately 16:50:16, for about 1 hour 33 minutes 26 seconds of
ordinary use. The transient startup artifact did not prevent that session.

The game rewrote the current full settings file at 17:31 during the
user-controlled run; its post-run SHA-256 is
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.
All 48 allowlisted graphics, display, and performance values still match the
committed
[M4/MoltenVK 1.4.2 standard template](../config/usersettings-m4-moltenvk-1.4.2-standard.txt).

Experiment 0028 now implements the bounded input-provenance successor. It
retains the exact aggressive profile and the twenty pixel/draw samples while
adding ordered descriptor-layout identity, required bound-set handles, the
latest descriptor update batch, dynamic offsets, and push-constant hashes to
the same submit-semaphore/present provenance chain. Identical repeated updates
remain identical; missing required updates, layouts, pushes, capacity, or
present provenance fail closed.

The synthetic sampled-image/push case, real official MoltenVK/AppKit
zero-input pipeline at small and exact ESO extents, dedicated analyzer
controls, clean full bridge build, configuration probe, and all 114 Python
tests pass. No game process was launched. The active cache
`a9a4ee5112466265c233e2561bb6c284032bfa30f077c0227259f09db682a063`,
old-backup cache
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`,
and settings
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`
were recorded before the cache-preserving restore and clean build. The
approved experimental install and exact installed-state verification remain
before any user startup.
The full settings and latest logs remain ignored evidence at
`artifacts/experiment-0021-post-validation-20260801T052538Z`.

At the earlier Experiment 0025 checkpoint, the active 1.4.2 cache was
7,977,079 bytes, had UUID
`db6602241a0502090000000100000000`, and SHA-256
`aed8bce13b26a8d2760b69d34440d27b1bdf244b4cdee6490bd847f759b904ba`.
ESO updated its contents during that run without changing its size. The
old-backup
cache likewise remained
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`.
The preserved 1.4.1 runtime/cache and older cache remain available.

## Historical safety record

The following entries preserve the earlier investigation state and must not be
read as the current recommendation. At the start of this record, the bridge was
validated only for a short world interval with ambient occlusion disabled.
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
audit.

The final source and compatibility gates now pass from commit `2f14d56`.
A clean source build reports MoltenVK 1.4.1 and pipeline-cache UUID prefix
`0DB445FF`; the exact runtime SHA-256 is
`9f7cf026c70c572dc4ab8709dc6e5ee60fadd9aba2a0f7b5f4bd5bade492549f`
and the proxy SHA-256 is
`422b4398742bc1d7ab4451bf5957467c514420d77c4dfe73756b061ea83f3b0f`.
The differential drawable probe, filtered device/proc probe, filtered
surface-format probe, embedded-runtime baseline probes, warnings-as-errors
build, static analysis, and all 69 Python tests pass. The selected ESO
fingerprint and pristine restore source are current; both caches and settings
remain preserved. No ESO, Steam, or launcher process was found at the last
read-only check.

The user explicitly approved the exact one-patch candidate. At approximately
2026-07-25 23:53 KST, Experiment 0016 was restored with both caches preserved;
the original-loader and target gates passed; the bridge was rebuilt against
the real original loader; and Experiment 0017 was installed in exact
`texture-cache-fix` mode.

Post-install checks report the target current, marker present, installed proxy
and MoltenVK byte-identical to the prepared candidates, both pipeline caches
and `UserSettings.txt` unchanged, quick update gate `READY`, and ESO, Steam,
and the launcher stopped. The ignored evidence boundary is
`artifacts/experiment-0017-20260725T145409Z`. The heavy Experiment 0016 audit
is no longer active. The remaining gate is one user-controlled,
normal-performance world entry and one resolution reset.

The user completed that gate in run
`20260725T145545.144942000Z-pid69302`. FPS was restored, confirming that the
Experiment 0016 audit was the source of the severe progressive slowdown. A
single 2048 x 1280 to 1920 x 1200 change nevertheless produced full black
output. The macOS cursor remained visible and keyboard/mouse input continued
to produce game-audio responses. No second run was requested.

All 51 evidence checksums verify. Startup passed in exact
`texture-cache-fix` mode; the two settings dimensions were the only
`UserSettings.txt` changes; no crash, Vulkan failure, device loss, or
swapchain-lifecycle anomaly occurred. Replacement generation 3 acquired and
presented 797 frames before orderly destruction.

Most importantly, every captured ESO swapchain image view was a 2D,
format-identical, identity-swizzle, full-range color view. MoltenVK sets
`_useMTLTextureView=false` for that case and fetches the current drawable
texture directly. The `9a5e233` repair executes only for a cached derived
Metal texture view. The non-game probe now proves the boundary automatically:
official 1.4.1 and the patch both pass the identity-view arm, while only the
swizzled arm distinguishes official `STALE` from patched `PASS`.

Experiment 0017 therefore failed as an ESO repair and excludes this exact
internal texture-view cache path. Its valid upstream bug reproduction is
retained. The bridge remains installed and recoverable, and both caches remain
preserved. Do not repeat Experiment 0017.

The active work is source-only classification of the remaining MoltenVK 1.4.2
changes plus focused non-game reset probes. ESO enabled only
`VK_KHR_swapchain`, `VK_KHR_maintenance1`, and `VK_EXT_debug_marker`;
argument buffers remain disabled; antialiasing is disabled. Descriptor
alignment/indexing, external-memory, private primitive-restart, and the
observed-not-1x1 swapchain fix are therefore not leading paths. Before another
game-bundle change, isolate the remaining memoryless-attachment,
render-pass dependency/resolve encoder, and visibility-query changes and
account for the 1.4.2 pipeline-cache UUID transition without deleting either
cache.

That classification is now complete. An exact source-built 1.4.2 and official
1.4.1 expose identical core feature bits, limits, and sparse properties under
the bridge's argument-buffer-disabled configuration; only API/driver version
and pipeline-cache UUID differ. The known 1.4.2 changes target extensions,
argument-buffer state, transient or multisampled images, heap placement,
visibility queries, or the derived swapchain-view path that do not match the
captured ESO reset. A new 24-cycle non-game composite probe also passes on both
versions while recreating exact-size render targets, updating one full-lifetime
descriptor set, and alternating command-buffer and command-pool reset. There is
no reset-relevant differential basis for replacing the whole runtime with
1.4.2.

A different compatibility delta is confirmed. On the Apple M4, embedded
MoltenVK 1.0.18 reports 18 enabled Vulkan 1.0 core features while official
1.4.1 reports 36. ESO's static device-init routine checks ten features that
are true in both runtimes, then queries the complete feature structure again
and passes it unchanged as `VkDeviceCreateInfo.pEnabledFeatures`. The bridge
therefore activates 18 capabilities that the embedded runtime kept disabled.

Experiment 0018 is the next narrow repair candidate. Its
`legacy-feature-profile` mode masks exactly those 18 additions at the
`vkGetPhysicalDeviceFeatures` GIPA route and fails closed at `vkCreateDevice`
if any is re-enabled. It does not spoof properties, limits, vendor identity,
or pipeline-cache UUID and does not add a hot-path audit. The automated M4
gate proves that all 55 visible feature values then exactly match embedded
1.0.18 and that real ESO-era device creation succeeds.

The source candidate has completed a clean shadow-bundle rebuild and the full
non-game M4 gate without touching the installed Experiment 0017 bundle.
Prepared official-1.4.1 proxy and runtime hashes are recorded in the
[Experiment 0018 record](experiments/0018-legacy-feature-profile.md). All
static checks pass and source commit `26d26ac` fixes the candidate identity.
After explicit approval, Experiment 0017 was restored and Experiment 0018 was
rebuilt from the real pristine loader and installed in exact
`legacy-feature-profile` mode. Both pipeline caches and `UserSettings.txt`
retain their pre-install hashes; the installed artifacts match the prepared
hashes; the target and marker checks pass; and the quick update gate is
`READY`. The ignored evidence boundary is
`artifacts/experiment-0018-20260725T154658Z`. The remaining gate is one
user-controlled normal-performance world entry and one resolution reset.

That gate failed. The exact run
`20260725T154920.411050000Z-pid85904` passed startup with the 18/18 mask and
created the device with exactly the embedded feature profile. A single
1920 x 1200 to 2048 x 1280 change produced solid-color output. All 48 evidence
checksums verify; only those two settings values changed; no lifecycle anomaly
occurred; and generation 3 acquired and presented 313 further frames.
Experiment 0018 therefore excludes the added core-feature category.

The same run exposes a performance-path defect in the bridge. Every generation
3 acquire and present returned `VK_SUBOPTIMAL_KHR`; the lifecycle wrapper
classifies that as non-success and therefore emitted and flushed 626 per-frame
records after the reset. The next source-only candidate is one combined
`performance-safe` path: remove the failed feature mask and all lifecycle
wrappers, enable asynchronous queue submission and concurrent compilation,
while retaining official MoltenVK 1.4.1, both HDR filters, disabled argument
buffers, live-resource compatibility, MTLHeap, command pooling, and no
prefill. Do not request a user run until this bundle is fully rebuilt and
validated.

Experiment 0019 now implements that combined `performance-safe` source path.
All twelve lifecycle-observed GDPA functions return their original MoltenVK
pointer, the failed feature mask is inactive, asynchronous submission and
maximum concurrent compilation are enabled, and live-resource checking remains
enabled. The effective configuration probe, direct-routing unit gate, complete
shadow-bundle source build, 73 Python tests, static checks, and Clang analysis
pass. The M4 reset composite also passes all 24 cycles with the combined
submission and compilation settings. Prepared hashes and the remaining
installation gate are recorded in the
[Experiment 0019 record](experiments/0019-performance-safe-path.md). No game
bundle file had been changed at that preparation stage. The candidate source
identity is commit `ffcf3e5`.

After explicit approval, Experiment 0018 was restored to the pristine loader
with both pipeline caches preserved. The proxy was rebuilt from that real
loader and reproduced the prepared hashes. Experiment 0019 is now installed
with marker `performance-safe`; the installed proxy and official MoltenVK 1.4.1
are byte-identical to the build. The target fingerprint, pristine loader, both
pipeline-cache hashes, and settings hash passed post-install verification.
ESO, Steam, and the launcher remained stopped. The ignored evidence boundary is
`artifacts/experiment-0019-20260725T161733Z`. The only remaining gate is one
user-controlled world entry and one fullscreen-resolution reset.

That gate failed for rendering correctness. Exact run
`20260725T162124.599998000Z-pid90854` passed the startup checker with the
intended configuration and direct MoltenVK routing for all twelve former
lifecycle wrappers. It emitted no lifecycle hot-path record, reported no
Vulkan failure or crash, and changed only the requested resolution from
2048 x 1280 to 1920 x 1200. The user nevertheless observed solid-color output.
All 48 evidence checksums verify. The user reported no obvious other problem
with the performance build, but this was not a controlled performance
measurement.

Experiment 0019 therefore closes asynchronous submission, concurrent
compilation, and diagnostic hot-path removal as a reset repair. Keep the
installed `performance-safe` configuration as the current low-overhead
MoltenVK 1.4.1 checkpoint. Continue performance work independently; do not
request another game execution merely to subdivide the failed repair bundle.

Experiment 0020 now implements the remaining `performance-aggressive`
derivative, changing only live-resource checking from `1` to `0`. MoltenVK
1.4.1 source analysis confirms that the safe setting performs a locked live-map
lookup when changed textures, buffers, and samplers are encoded with argument
buffers disabled.

A balanced non-game M4 probe measured 21 samples per setting over 20,000
alternating-resource draws. Median synchronous CPU submit encoding fell from
195.833 to 176.090 ns per draw, a 10.082% reduction in that descriptor-heavy
interval. This is not an ESO FPS claim. The exact aggressive asynchronous
profile passes the 24-cycle reset composite, Metal pixel validation, ESO-era
device creation, proc profile, and HDR surface filter. The full build, 80
Python tests, and static analysis pass.

After explicit approval and confirmation that Steam, the launcher, and ESO were
stopped, Experiment 0019 was restored with settings and both pipeline caches
preserved. The proxy was rebuilt from the actual pristine loader and reproduced
the prepared hashes. Experiment 0020 is now installed with marker
`performance-aggressive`; the installed proxy, renamed original, and official
MoltenVK 1.4.1 are byte-identical to the rebuild. The active cache, old cache,
and settings hashes are unchanged. The ignored post-install evidence boundary
is `artifacts/experiment-0020-20260725T163919Z`.

The install is verified, but an ESO FPS increase is not claimed. Ordinary use
is the remaining validation boundary; Experiment 0020 does not request another
graphics reset. Its measured result, risk boundary, exact hashes, installation,
and rollback state are in the
[Experiment 0020 record](experiments/0020-live-resource-performance.md).
The candidate source identity is commit `ed5b9d3`.

The official MoltenVK 1.4.2 release has now been reconsidered as a maintenance
upgrade rather than only as a proposed reset repair. Its official macOS archive
and universal runtime hashes were verified, and that release passed the
complete shadow build plus the exact Experiment 0020 configuration, all
24 reset-composite cycles, ESO-era device/proc creation, both HDR filters, and
Metal pixel checks on the Apple M4.

The release adds no proven repair for ESO's solid-output reset. The cached
derived swapchain-view fix was already excluded by Experiment 0017, and the
captured reset-created images do not use the transfer-source combination
addressed by another Apple Silicon fix. However, the new subpass-dependency
Metal barrier is below the existing public-state audit and cannot be excluded,
and 1.4.2 removes several other general correctness defects.

A balanced descriptor-heavy CPU benchmark measured 176.068 ns/draw on 1.4.1
and 180.105 ns/draw on 1.4.2, a 2.293% difference below the existing 3%
meaningful-change threshold. This is neither an FPS gain nor a material
measured regression.

The maintenance recommendation is therefore to prepare official 1.4.2 with
the unchanged `performance-aggressive` profile. Its distinct pipeline-cache
UUID requires preserving the active 1.4.1 cache under a versioned name and
starting a separate cold 1.4.2 cache; the pre-1.4.1 backup must remain
untouched. No game-bundle modification was made by this review. Full evidence
and limits are in the
[MoltenVK 1.4.2 adoption review](research/moltenvk-1.4.2-adoption-review.md).

Experiment 0020's ordinary-use validation is now complete. The exact startup
run passed, reached the loaded-world interface state, produced no crash report,
and the user reported no problem. The user-directed outcome is **succeeded**.
This validates ordinary use of the installed profile; it is not a controlled
FPS measurement and does not supersede the known graphics-reset failure.

The user explicitly approved official MoltenVK 1.4.2 adoption as Experiment
0021. The fetch, build, and selected target now pin the official archive
`f95765a6...` and universal runtime `aef00b13...`. A new fail-closed cache
identity tool validates the complete Vulkan cache header, and the installer
requires the exact 1.4.1 runtime and UUID before preserving them under
versioned names. It leaves the older cache backup untouched and starts 1.4.2
without an active cache.

A fresh shadow source build, 87 tests, static analysis, exact-profile
configuration checks, 24-cycle M4 Metal reset comparison, device/HDR probes,
and a complete shadow install/restore all pass.

After Steam, the launcher, and ESO were confirmed stopped, Experiment 0020 was
restored without changing settings or either cache. The proxy was rebuilt from
the actual pristine game loader at source commit `79bb444` and reproduced the
prepared hashes. Experiment 0021 is now installed with official MoltenVK 1.4.2
and the unchanged `performance-aggressive` marker. Installed runtime and proxy
hashes match the rebuild.

The 4,259,071-byte 1.4.1 cache and exact 1.4.1 runtime are preserved under
versioned names; the 6,800,792-byte older cache remains unchanged. The active
cache is intentionally absent so 1.4.2 starts cold, and the settings hash is
unchanged. Target, process, cache UUID, and post-install gates all pass. The
ignored evidence boundary is
`artifacts/experiment-0021-20260725T170433Z`. Shader compression remains a
separate downstream change. See
[Experiment 0021](experiments/0021-moltenvk-1.4.2-maintenance.md).
