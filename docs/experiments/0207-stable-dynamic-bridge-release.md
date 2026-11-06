# Experiment 0207 — Stable dynamic-bridge baseline 0.3.0

Date: 2026-11-06. Scope: user-authorized stable promotion of public RC1's exact
native payloads, with manual installation after compatible official updates.

## User decision and architectural meaning

The user clarifies the intended architecture: preserve a bridge outside the
updated package, retain a reliable per-launch loading route, and let that bridge
rediscover compatible connection points. If all three survive an update,
reinstallation is unnecessary. Automatic recovery is a fallback strategy, not
the definition of dynamic bridging. That full loading architecture is unproven.
Do not present the earlier recovery research as delivered product functionality.

The user selects the available dynamic bridge plus manual reinstallation as the
release baseline now. Stable 0.3.0 retains the existing disclosed intermittent
focus issue; accepting it for this release is not a fix or new gameplay result.

## Immutable promotion and source isolation

Source base: public main commit 993673171be1bffc2ae798166d1df526aabe03d0,
which differs from v0.3.0-rc.1 only by publication evidence. Native and installer
source do not change. Export that public source without replacing the local
research checkout. Do not publish private runtime-attestation, Steam wrappers,
Starter diagnostics or the local research commit series in this release.

Download the public RC1 ZIP and verify its SHA-256
 d2bed3a1d4049a6327ab2b1d7c2e49cbc70ff34fb617c30cd211d2a9e81d43c6.
Promote only README.txt, RELEASE_VERSION in target-profile.env, and the package
checksum manifest. Every other file remains byte-identical, including installer
logic, bridge, universal native auditor, runtime and visible commands. Package
root changes to the final version. No native rebuild is substituted for the
previously user-tested bytes, so no new gameplay request is needed.

| Artifact | SHA-256 |
| --- | --- |
| Final ZIP | fcae1194bb1e63ae444ad95d049e25e35825819b3782393c89895ee3293fdb32 |
| Bridge | 4a204907c8236e8e87fcd790a9ae849e3a1d43f4704b927cfa79be8b7233247c |
| Native auditor | c6a37ef409ffd52b82736ec4443576a86968935390dc20528c1fd1f15fba3141 |
| Official MoltenVK 1.4.2 | aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f |

ZIP name: ESO-MoltenVK-Patcher-0.3.0.zip; size: 3,476,618 bytes.
No proprietary original Bink, game executable, user settings, cache or crash
report is included. The archive's three changed-file identities are checked.

## Verification

All 154 public-baseline Python tests pass in 6.252s. The exact public installer
fixture passes install/remove/reinstall, interruption, generation and settings
preservation controls. Clean archive checks pass executable modes, visible
layout, checksums, LF and privacy/metadata hygiene. Freshly compiled owned smoke
probes resolve BinkOpen from the promoted bridge and show Rosetta patch 1 to 2.
The unchanged native auditor derives and verifies the current ESO 12.1.6 /
3309674 profile. An initial redirected test profile had mode 0644; verification
correctly refused it. Applying the build script's required private mode 0600 to
that temporary profile makes the unchanged verification pass. No comparison is
weakened. Production inputs are preserved and no game/Steam/launcher is started.

The original private 0079 installation is not replaced by publication. Prior
stable/prerelease tags and assets remain immutable. Manual package use preserves
original-loader generations/settings/caches under the existing installer rules.
Rollback means retaining older immutable release artifacts, not restoring an
obsolete original over a newer official client. No universal future compatibility,
FPS, focus or automatic-loading guarantee is claimed.

## Publication evidence

Publication metadata, final source commit/tag, CI and downloaded-asset verification
are recorded after successful upload. Until then this section is preparation
evidence, not a claim that the final release is publicly available.

## Publication confirmed — 2026-11-06

Annotated tag v0.3.0 peels to ec80f18f44e2152e24fd1607909a8a2140a59d5f,
from the public baseline plus documentation/version updates only. Release and
source CI pass:
[branch check](https://github.com/meringue5/eso-moltenvk-patcher/actions/runs/42156531892),
[tag check](https://github.com/meringue5/eso-moltenvk-patcher/actions/runs/42156590409).
The uploaded draft ZIP was downloaded, matched the exact final SHA-256 and
passed clean-archive checks before publication. GitHub now reports draft=false,
prerelease=false and latest=v0.3.0. Prior releases/assets are unchanged.

[Stable 0.3.0](https://github.com/meringue5/eso-moltenvk-patcher/releases/tag/v0.3.0)
is published at 2026-11-06T03:28:10Z. The ZIP is 3,476,618 bytes with server
digest sha256:fcae1194bb1e63ae444ad95d049e25e35825819b3782393c89895ee3293fdb32.
Local unpublished research was not pushed or packaged. All 34 production
preservation records match and no real install or app launch occurs.
