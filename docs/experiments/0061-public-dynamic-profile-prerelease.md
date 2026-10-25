# Experiment 0061: public dynamic-profile prerelease

- Date: 2026-10-25
- Outcome: **user-authorized; release gates and publication in progress**
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

## Publication

Pending final checks and remote confirmation. No release is claimed by this
preparation record until the publication amendment is added.
