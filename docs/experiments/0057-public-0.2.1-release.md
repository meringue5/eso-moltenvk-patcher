# Experiment 0057: public 0.2.1 release

- Date: 2026-10-05
- Outcome: **succeeded; released at the user's explicit request**
- Rollback: **public 0.2.0 remains published; local reinstall of any earlier
  ZIP or Uninstall restores the verified original**

## Question

Can the ESO 12.1.5 support in Experiment 0054 ship publicly without
including runtime behavior that is still unproven?

## Decision

The user requested a public, non-RC release so players can use the patcher
after the 2026-09-30 update. The release keeps exactly the runtime the user
launched as `0.2.1-rc.3`: 0.2.0 runtime control, the 12.1.5 target, and
observation-only activation records.

The Experiment 0056 in-process activation fallback (`rc.4`) is reverted on
both `release/0.2.1` and `main`, because its first real request did not
activate ESO. The activation hand-off failure (Experiments 0052, 0055, 0056)
is published as a known issue, not as fixed.

## Identity

- Tag `v0.2.1` → release commit `616fcb693138659fee93f1563b68be67206821a2`
  on `release/0.2.1`, merged into `main`.
- `ESO-MoltenVK-Patcher-0.2.1.zip`, 3,420,086 bytes, SHA-256
  `5cfae50224a91410de4ff7f3bd64c73047402c8c6b9d195015da5249b6619d54`.
- Bridge SHA-256
  `c025c5bef5db70fad9b9ff3d6fe88b04c082962a1942b62d23789b1cb3fbc1f3`. This is
  byte-identical to the rc.3 bridge from the 2026-09-30 user launches
  (Experiment 0055).
- Target: ESO 12.1.5 / databuild 3303624, executable SHA-256
  `027d5a6d...0a26`, original Bink `9a0872f4...6de8`.

## Gates

- `check-update.sh` reports `CURRENT`.
- Fresh package build: Bink re-export, Rosetta self-patch 1 to 2, inactive
  pacing smoke, and all MoltenVK configuration probes PASS.
- Installer transaction fixture PASS, release archive PASS, and 143 Python
  tests PASS on the release branch. Static checks PASS.
- The final ZIP was installed in place over rc.4 with `--skip-settings`
  after the exact-target and idle gates. Status reports `READY` and
  `Installed release: 0.2.1`. `UserSettings.txt` stayed `788c34ec...`, all
  pipeline-cache identities pass, and the recovery backup verifies.
- Diagnostics export: a 0600 ZIP with no home path or account identifier,
  reporting `Installed release: 0.2.1`.

## Runtime evidence carried forward

The rc.3 bridge's user launches on 12.1.5 had all 17 redirects, exact
79/150/180 startup control, and no bridge error. Once ESO was active, the user
observed 60 FPS. Two of the four launches started inactive. That is the known
launcher hand-off issue, which also occurs without the patch.
