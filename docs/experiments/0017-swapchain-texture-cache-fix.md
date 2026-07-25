# Experiment 0017: swapchain Metal texture-cache fix

- Date: 2026-07-25
- Outcome: **failed; normal FPS restored but live resolution reset blacked out**
- Rollback: **checked pristine loader available; not performed**

## Question

Does MoltenVK 1.4.1 keep a `VkImageView`'s cached Metal texture view attached
to an earlier `CAMetalDrawable` after the same swapchain `VkImage` is
reacquired with a different drawable texture, and does the exact upstream fix
repair that state without replacing the rest of 1.4.1?

## Hypothesis

MoltenVK 1.4.1 lazily caches the `MTLTexture` created behind an image view. A
presentable swapchain image obtains its base texture dynamically from
`CAMetalLayer.nextDrawable`, but the cached view is not invalidated when that
base texture changes. Commands can therefore encode successfully against an
old drawable while acquire, submit, and present continue on the new one. This
directly permits a stale last frame or undefined solid output with otherwise
valid public Vulkan state.

Upstream commit
[`9a5e233`](https://github.com/KhronosGroup/MoltenVK/commit/9a5e233ef08e3a0f58c7b90053385cfb5cacde68)
compares the current base texture, parent/root texture, and IOSurface identity
before returning the cached view, and releases and recreates the view when the
base changed. It was included in MoltenVK 1.4.2. This experiment backports only
that commit to the exact 1.4.1 source tag; it is not a wholesale 1.4.2 upgrade.

## Target and change set

- Base source commit:
  `db445ff2042d9ce348c439ad8451112f354b8d2a` (`v1.4.1`).
- Upstream fix commit:
  `9a5e233ef08e3a0f58c7b90053385cfb5cacde68`.
- Exact committed patch SHA-256:
  `4d31f4ce6175935e2208061e800c57ddf2c47679ca6d91384768ae7527386686`.
- Build an x86_64 Release dylib with the v1.4.1 CMake dependency revisions.
- Run CMake from the MoltenVK source root and require the generated revision
  header to contain `db445ff`. MoltenVK places this value in its pipeline-cache
  UUID; allowing CMake to inherit the bridge repository's working directory
  would spuriously change cache identity on every bridge commit.
- Keep the established descriptor-compatible runtime configuration: live
  resource checking enabled, Metal argument buffers disabled, MTLHeap
  `where safe`, synchronous submission, command pooling enabled, and command
  prefill disabled.
- Select a distinct `texture-cache-fix` marker mode.
- Do not enable the full-lifetime render audit or its hot-path tables.
- Retain only the existing bounded, cold swapchain lifecycle records and add
  image-view type, component mapping, and subresource range to swapchain-view
  creation records.
- Preserve both pipeline caches and the user's settings.

## Non-game differential probe

`tools/probe_swapchain_texture_cache.mm` creates a 64 x 64 `CAMetalLayer`
swapchain on the Apple M4. Each swapchain image receives a component-swizzled
image view, which requires a Metal texture view. A render-pass clear writes a
different red component every frame. The probe then reads the first pixel from
the current drawable's base Metal texture before presentation.

The probe records the Metal texture identity for every acquired Vulkan image.
It is conclusive only after the same `VkImage` receives a different base
`MTLTexture`:

- if the current clear reaches the replacement texture, the pixel contains the
  exact current-frame value;
- if the image view still targets the earlier texture, the new drawable pixel
  remains untouched and the probe returns `STALE`;
- no observed base-texture replacement is inconclusive.

On this M4, the official release MoltenVK 1.4.1 binary reproduced the defect.
At frame 3, image 0 changed base texture identity and the expected variable
byte was 82, but the current texture read `0,0,0,0`. Later uses of that image
remained untouched. The probe returned exit 3 and `STALE`.

The exact 1.4.1 source build with only the upstream patch saw the same
base-texture replacement at frame 3. It read `191,64,82,255`, and all later
frame values matched. The probe returned exit 0 and `PASS`.

`scripts/probe-swapchain-texture-cache.sh` builds both probes with
warnings-as-errors and requires this exact differential result. The test
executes MoltenVK and Metal only; it does not launch Steam, the launcher, or
ESO.

## Pre-install gate

- Commit the source candidate before the final source rebuild.
- Run `scripts/check-update.sh` and require the exact ESO fingerprint.
- Build the patched MoltenVK from the exact source and patch identities above.
- Re-run the differential M4 probe: official 1.4.1 must return `STALE`, and
  the backport must return `PASS`.
- Run the complete teso4m4 source build against a checked pristine loader,
  plus Python tests and compilation, shell syntax, whitespace checks,
  warnings-as-errors, and Clang static analysis.
- Run all replacement and embedded-runtime M4 compatibility probes.
- Before any game-bundle write, require ESO, Steam, and the launcher stopped;
  recheck the original loader, pristine restore source, target fingerprint,
  caches, settings, candidate hashes, and restore path.
- Installation requires new explicit user approval. Experiment 0016 consumed
  the earlier one-candidate approval.

## User procedure

This is the remaining planned user-controlled execution and is a repair
validation, not another diagnostic-only run:

1. launch once through the normal Steam path and enter the existing world at
   normal performance;
2. change fullscreen resolution once;
3. confirm only whether rendering continues normally, becomes solid, freezes,
   or crashes, then exit.

No travel, HUD, screen capture, FPS log, additional setting change, or extended
play is requested. If the launch enters the known low-performance state, do not
immediately repeat it; collect and analyze that run first.

## Pass/fail

- **Non-game pass:** official 1.4.1 reproduces stale output after a real
  drawable texture replacement, and the exact one-patch backport writes the
  current drawable correctly.
- **Rendering pass:** after normal-performance world entry, one graphics reset
  preserves correct rendering.
- **Fail:** the known solid-color or frozen-frame failure recurs with the
  backport active.
- **Inconclusive:** the target or candidate hash changes, the bridge mode or
  startup verdict is wrong, the run is limited to the low-performance state,
  or the requested reset is not observed.

## Evidence

The official and patched source-built dylibs both report MoltenVK 1.4.1 and are
x86_64. After install-name normalization, the final prepared runtime with the
pinned `db445ff` pipeline-cache revision has SHA-256:

```text
9f7cf026c70c572dc4ab8709dc6e5ee60fadd9aba2a0f7b5f4bd5bade492549f
```

The differential probe has passed against the official release archive and
that candidate on the Apple M4. The selected ESO 12.0.7 SHA-256, UUID,
client version, and databuild remain current. No game-bundle file has been
changed for Experiment 0017.

## Result

Source commit `2f14d56` completed a clean bridge rebuild against the checked
pristine loader without writing the game bundle. The final proxy SHA-256 is:

```text
422b4398742bc1d7ab4451bf5957467c514420d77c4dfe73756b061ea83f3b0f
```

The complete warnings-as-errors build, Bink re-export check, Rosetta
self-patch probe, compatibility smoke tests, lifecycle/reset/audit probes, all
nine effective MoltenVK configuration modes, 69 Python tests, Python bytecode
compilation, shell syntax, whitespace checks, and Clang static analysis passed.

The final M4 gates then reconfirmed:

- official 1.4.1: `STALE` after a real drawable texture replacement;
- exact one-patch candidate: `PASS` after the same replacement;
- MoltenVK version: `1.4.1`;
- pipeline-cache UUID prefix: `0DB445FF`;
- HDR extension filter: 131 raw, 130 visible, exact one removal;
- surface-format filter: 60 raw, 59 visible, exact one removal;
- ESO-era device creation and all 100 probed proc routes: expected result.

The original embedded 1.0.18 Vulkan and surface probes also retained their
recorded baseline. At the final preflight checkpoint, no ESO, Steam, or launcher
process was found; the selected ESO fingerprint was current; the pristine
loader existed with SHA-256
`c269d54e23a0669037df39a77386f0b5e380f715d4416091d028ab9ca20802eb`;
and both pipeline caches and the settings file existed and were fingerprinted.

At approximately 2026-07-25 23:53 KST, after explicit user approval, the
Experiment 0016 bridge was restored with both pipeline caches preserved. The
post-restore gate required the exact ESO fingerprint, original loader, absent
marker, unchanged settings and cache hashes, and the checked pristine loader.

The bridge was then rebuilt from source against that real original loader and
reproduced the prepared proxy and MoltenVK hashes exactly. Installation in
`texture-cache-fix` mode preserved both pipeline caches. Post-install checks
confirmed:

- exact ESO 12.0.7 fingerprint and UUID;
- bridge target current and marker exactly `texture-cache-fix`;
- installed proxy and MoltenVK byte-identical to the built candidates;
- both pipeline caches and `UserSettings.txt` unchanged;
- quick update gate `READY`;
- ESO, Steam, and the launcher still stopped.

The ignored evidence boundary is
`artifacts/experiment-0017-20260725T145409Z`.

The user then launched once through the normal Steam path. The exact selected
run is `20260725T145545.144942000Z-pid69302`. Performance had recovered from
the Experiment 0016 low-FPS state. Changing fullscreen resolution from
2048 x 1280 to 1920 x 1200 produced a full black-out. The macOS cursor remained
visible, and keyboard and mouse input still caused expected game-audio
responses. The user exited without another run.

Collection and analysis completed successfully:

- all 51 evidence-file checksums verify;
- the startup verdict passed in exact `texture-cache-fix` mode;
- no crash report, Vulkan failure, lifecycle anomaly, or device loss appeared;
- `UserSettings.txt` retained exact structure and changed only
  `FullscreenWidth` and `FullscreenHeight`;
- the terminal reset completed `DeviceWaitIdle`, swapchain creation, and
  `OnDeviceReset`;
- replacement generation 3 acquired and presented 797 frames before orderly
  destruction;
- both generation-3 swapchain image views were 2D, used format 44, identity
  component mappings, color aspect, and the complete remaining mip/layer
  ranges.

The continuously returned `VK_SUBOPTIMAL_KHR` result is not promoted to a new
cause. Experiment 0010 already forced out-of-date results without inducing
recreation and separately eliminated suboptimal presentation through scaling
without correcting the corrupt output.

The non-game probe now has an explicit identity-view arm matching the captured
ESO view. After a real drawable texture replacement:

- official 1.4.1 with the swizzled derived view still returns `STALE`;
- the one-patch candidate with the swizzled derived view returns `PASS`;
- official 1.4.1 with the identity view returns `PASS`;
- the one-patch candidate with the identity view returns `PASS`.

MoltenVK's `MVKImageViewPlane` sets `_useMTLTextureView` to false when the view
format, texture type, complete subresource range, and swizzle are identical to
the underlying image. In that case `getMTLTexture()` directly fetches the
presentable image's current drawable texture. Upstream commit `9a5e233` only
invalidates the cached object inside the `_useMTLTextureView` branch.

## Interpretation

The original non-game result remains a valid MoltenVK 1.4.1 defect and the
backport remains its valid repair. The ESO run disproves applicability to this
failure: ESO's direct identity swapchain views never enter the cached derived
view branch changed by `9a5e233`, and rendering still blacked out.

Experiment 0017 therefore closes as a failed ESO repair, not an inconclusive
test. It excludes this specific MoltenVK internal cache path. The restored FPS
also supports the prior conclusion that Experiment 0016's full-lifetime audit,
not the ordinary 1.4.1 bridge configuration, caused the severe progressive
performance degradation.

## Rollback

Experiment 0017 is installed. The checked pristine Bink loader remains the
restore source, the Experiment 0016 marker was displaced under its timestamped
name, and no cache or setting was deleted or replaced. No rollback was
performed after the failed run. Preserve this checkpoint until a fully
validated successor requires a clean source rebuild; do not request another
user run against Experiment 0017.
