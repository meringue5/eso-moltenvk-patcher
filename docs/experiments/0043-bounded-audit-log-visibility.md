# Experiment 0043: bounded compositor audit log visibility

- Date: 2026-08-27
- Outcome: **succeeded; GUI-classified input contains in-place magenta**
- Rollback: **not started; Experiment 0043 remains installed and active**

## Question

Can the already-running forward-only compositor audit preserve its bounded
pixel, draw, descriptor, and two-input image evidence under the production
default `info` log level, without enabling unbounded proc or lifecycle trace?

## Evidence selecting this gate

Experiment 0042 exact run `20260827T053611.563842000Z-pid26073` selected the
correct mode and produced normal FPS with visible pink, but the analyzer found
none of its required audit records. The log classifier assigns generic
`STARTUP_PRESENT_*`, `STARTUP_DRAW_*`, `STARTUP_INPUT_*`, and compositor-image
messages to `debug`, and assigns `STARTUP_COLOR_*` to `trace`. The default
production level is `info`, so the evidence was deterministically filtered.

## Controlled change

Keep the complete Experiment 0042 runtime, inactive pacing bypass, MoltenVK
configuration, cache state, settings, draw forwarding, sampling schedule, and
analyzer unchanged. Promote only these already-bounded audit families to
`info`:

- color-audit begin and finish;
- `STARTUP_PRESENT_*`;
- draw-audit begin;
- input-audit begin;
- compositor-audit begin; and
- `STARTUP_COMPOSITOR_IMAGE_*`.

Generic `GIPA`, `GDPA`, detailed color-clear records, and other trace/debug
families remain filtered. Error classification remains unchanged.

## Gates

Before installation, require a fresh complete bridge build, lifecycle and mode
probes, analyzer regression, all Python tests, release transaction regression,
static checks, and the official/embedded Metal-backed non-game probes. Install
only after ESO and the launcher close and the shared bundle-idle gate passes;
preserve all settings and caches.

## Planned user gate

One ordinary Steam-path launch with a report of pink visibility and FPS state.
Require the exact mode, all bounded begin/ready/finish markers, twenty aligned
samples, no audit error or overflow, and a decisive compositor-input verdict.

## Result

The logging policy is now a separately compiled module with a dedicated probe.
The probe directly verifies that every analyzer-required bounded family is
`info`, detailed color/GIPA records remain `trace`, generic lifecycle detail
remains `debug`, and error/warning precedence remains intact.

```text
fresh complete bridge build: PASS
bounded log-policy probe: PASS
  bounded audit=info, detail=trace, generic=debug
Bink re-export and Rosetta self-patch: PASS
inactive pacing and lifecycle/image probes: PASS
combined MoltenVK mode configuration: PASS
Python tests: 135 PASS
release installer transaction regression: PASS
Python compile, shell syntax, git diff check: PASS
official MoltenVK 1.4.2 Metal compatibility/surface probes: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probes: PASS on Apple M4
bridge SHA-256: 13cfbe01e6427b26f5a3a1dbf36a85627e35014324d1770315406824191f5d34
retagged original Bink SHA-256: f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

After ESO and the launcher closed, the shared bundle-idle gate allowed the
cache-preserving transaction because Steam had no ESO file or update activity.
The installed 0042 bridge was first restored to the verified pristine loader.
The active pipeline cache, old-runtime cache backup, and `UserSettings.txt`
then retained their exact pre-restore identities:

```text
active pipeline cache SHA-256: bdbfcb286b72fbfa842fedc5484e54af2e37851e8eaaeb731630d31e5a3c0807
old-runtime cache backup SHA-256: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
UserSettings.txt SHA-256: 104e894803e70dae30fdab887474a8f3116387375614484d36c3755c58745fb0
```

The same idle gate passed again for installation. Post-install status recognizes
the exact ESO 12.0.8 target, the installed bridge and MoltenVK as current, all
three preserved pipeline-cache headers as valid, and the marker as
`startup-compositor-audit-pacing-bypass`. The bridge re-exports the retagged
original Bink. Installed payload hashes exactly match the built candidates
above, and all three user-file hashes remain unchanged. No game or launcher was
started by the agent. One ordinary user-controlled Steam-path launch was the
remaining gate at this installation boundary.

The user then performed one ordinary Steam-path launch and reported normal FPS
with visible pink. Exact run `20260827T054945.100479000Z-pid33219` selected the
combined audit/pacing mode, activated all 17 redirects, logged the inactive
pacing state as `active=yes`, completed the bounded ordinal-180 audit, and
retained 64 successful non-null graphics-pipeline calls. No audit error, skip,
overflow, or lifecycle error was recorded.

The dedicated analyzer returned the decisive verdict:

```text
startup-compositor-audit-verdict: COMPOSITOR-GUI-MAGENTA-IN-PLACE-CONTENT-CHANGE
magenta output frames: generation 2 ordinals 80,90,100,110,120,130,140
normal-scene output frames: generation 2 ordinals 150,160,170,180
Sampler0/scene-classified input near-magenta: 0/5 at every compared frame
Sampler1/GUI-classified input near-magenta: 5/5 in every magenta frame,
                                            0/5 in every normal-scene frame
Sampler1 image signature: e39762f9424185a9 in both intervals
```

At ordinal 80, the second input at set 1 binding 2 is a full-surface BGRA8
image whose five sampled points are all exact magenta. At ordinal 150, the same
descriptor/image identity contains ordinary colors at all five points. The
first input at set 1 binding 1 is not magenta in either interval. The visible
pink therefore comes directly from the compositor's GUI-classified second
image, and the transition occurs by changing the contents of the same image
resource rather than replacing its descriptor identity.

This result does not identify the upstream command that writes the second
image. It does, however, remove the need to speculate between scene, GUI, and
combined shader output. A root repair would target that image's producer or
readiness transition. The immediate bounded cosmetic candidate can instead
combine Experiment 0031's already validated exact compositor window with
Experiment 0041's independent host-loop pacing bypass; it requires no image
readback or new resource substitution.

## Rollback

Not started. The pristine loader remains available and was verified during the
restore/install transaction.
