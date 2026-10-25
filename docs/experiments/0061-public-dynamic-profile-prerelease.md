# Experiment 0061: public dynamic-profile prerelease

- Date: 2026-10-25
- Outcome: **succeeded; public prerelease published and remote artifact verified**
- Rollback: **stable v0.2.1 and its immutable assets retained; verified
  current-generation removal available on the local installation**

## Decision and scope

After reviewing the two normal-launch results in Experiment
[0060](0060-dynamic-update-profile-implementation.md), the user explicitly
requested release now. Publish `v0.3.0-rc.1` as a prerelease, not latest stable.
Keep the tested bridge, native auditor and MoltenVK runtime unchanged; refresh
player documentation and package checksums for the new capability and limits.
Do not defer this release to add an unattended update/installation component.

The supported checkpoint is ESO 12.1.6 / databuild 3309674, Apple M4 and macOS
27.0.1. Dynamic profiles independently discover compatible Vulkan/host-pacing
sites at Install and re-discover/verify them at runtime. Changed executable
inputs still require explicit Install. A retained stale proxy requires vendor
Repair; unsupported original Bink generations or structural changes stop.
The vendor executable is never rewritten. Settings/cache policy and the normal
Steam authentication path remain unchanged.

Intermittent initial activation is disclosed: the first user launch reported
focus loss and approximately 40 FPS until an app switch; the second started
active normally. A first-run-only cause and a focus fix are unproven. Structural
startup PASS is not proof of visual focus or measured FPS. Stable promotion is
a separate acceptance decision; this prerelease does not erase the first
failure or promise compatibility with every future game update.

## Checks and identity

Fresh package build passes native current-target discovery/canonical verification,
Bink re-export, Rosetta self-patch (1 to 2), inactive-pacing RX restoration,
isolated probe logging and effective runtime configuration checks. All 154
Python tests pass. Installer transaction fixtures and clean-archive checks pass;
Python compilation, shell syntax, whitespace and 120 local documentation links
pass. The installed payloads and game files were not changed.

- ZIP `ESO-MoltenVK-Patcher-0.3.0-rc.1.zip`: 3,475,263 bytes;
  SHA-256 `d2bed3a1d4049a6327ab2b1d7c2e49cbc70ff34fb617c30cd211d2a9e81d43c6`.
- Bridge: `4a204907c8236e8e87fcd790a9ae849e3a1d43f4704b927cfa79be8b7233247c`.
- Universal native auditor: `c6a37ef409ffd52b82736ec4443576a86968935390dc20528c1fd1f15fba3141`.
- Official MoltenVK 1.4.2: `aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.

All three payload hashes match the package used for both user launches. Only
the player README/checksum packaging differs from the earlier local ZIP, which
is preserved in ignored evidence storage. No new game launch or real install
cycle is needed for documentation-only packaging changes. A separate SHA-256
asset accompanies the ZIP. Raw logs, game binaries and caches are not committed.

## Publication (confirmed 2026-10-25)

Annotated tag `v0.3.0-rc.1` peels to source release commit
`e48cda10b8f868709f8baaee6a64c4e19903c199`, pushed on `main`. The tag is
immutable; subsequent publication-evidence commits do not change its payload.
GitHub static checks passed for both
[main](https://github.com/meringue5/eso-moltenvk-patcher/actions/runs/40200615233)
and [tag](https://github.com/meringue5/eso-moltenvk-patcher/actions/runs/40200639540).
Those CI jobs check syntax and source hygiene; the native/runtime/transaction
and Python test results above are local macOS evidence, not CI gameplay tests.

The uploaded draft ZIP and SHA-256 asset were downloaded and checked against
the local artifact. The clean-extraction archive test passed on that download.
The release was then published at 2026-10-25 23:06:33 KST (14:06:33 UTC):
[v0.3.0-rc.1](https://github.com/meringue5/eso-moltenvk-patcher/releases/tag/v0.3.0-rc.1).
Remote metadata confirms `draft=false`, `prerelease=true`, ZIP size 3,475,263
bytes and server digest matching the SHA-256 above. The public asset is
[ESO-MoltenVK-Patcher-0.3.0-rc.1.zip](https://github.com/meringue5/eso-moltenvk-patcher/releases/download/v0.3.0-rc.1/ESO-MoltenVK-Patcher-0.3.0-rc.1.zip).
The latest-stable endpoint still returns `v0.2.1`; old releases/assets remain
unchanged. No real bundle reinstall, launch, settings/cache-policy change or
activation workaround was introduced for publication.

## Rollback and next gate

The local installation retains the same tested payloads with verified current
original recovery. Use the new package's generation-checked Remove path when
needed; do not install the old 12.1.5 package over the 12.1.6 client as a
compatibility workaround. For a release-blocking regression, mark this
prerelease as affected/withdraw its recommendation and retain its evidence;
never silently replace an uploaded immutable asset. Stable promotion requires
a separate scope/known-issue review. No repeated manual launch is requested.
