# Experiment 0046: original-loader generation-aware recovery

- Date: 2026-08-27
- Outcome: **succeeded in the disposable installer fixture**
- Rollback: **not applicable; the installed Experiment 0044 bundle was not changed**

## Question

Can Install, Uninstall, Status, and Repair avoid restoring an obsolete original
Bink backup after an ESO update while retaining safe repeated installation for
updates whose executable structure and original-loader generation are both
unchanged?

## Safety defect

The 0.1.2 release installer distinguished an exact executable from a
structurally compatible updated executable. When the update left the bridge in
the active Bink path, however, it restored the previously recorded original
before reinstalling. The active bridge hides the vendor's current original, so
that operation could not prove that the update had not advanced the original
Bink generation.

Uninstall used the inverse but equally incomplete rule: it required the current
ESO executable hash to equal the install record before restoring. It therefore
could not safely complete when the launcher had already restored or replaced
the active Bink after an update.

## Controlled change

Treat executable attestation and original-Bink SHA-256 as one recovery
generation across every command:

- an exact installed bridge with a matching marker and state may restore its
  same-generation backup;
- an executable update that leaves any bridge active stops Install and
  Uninstall without changing files and requires the ESO launcher Repair path;
- a launcher-provided non-bridge loader is never overwritten by the recorded
  backup;
- the same supported launcher-restored original remains eligible for a
  structurally compatible executable reinstall;
- a different original is rejected by an older release and preserved during
  Uninstall cleanup;
- a newer release that explicitly expects that different original archives the
  prior state, backup, and marker before making a fresh backup; and
- patch-owned renamed-Bink and MoltenVK companions are deleted only when their
  hashes match the current release payload/profile.

The source-maintenance Install and Restore scripts also verify that their
pristine backup matches the selected target's `original_bink_sha256`. Restore
refuses to replace an already active, different launcher-provided generation.

## Disposable fixture

The release transaction fixture now covers:

1. exact install, repeated install, remove, and reinstall;
2. interrupted installation and verified same-generation recovery;
3. settings merge, safe settings restore, and preservation of later changes;
4. a compatible executable update with the same launcher-restored original;
5. a compatible executable update with the bridge retained, where Status
   requires launcher Repair and both Install and Uninstall leave the bridge and
   backup byte-identical;
6. launcher Repair followed by Uninstall, which leaves the active original
   untouched;
7. a different launcher-provided original rejected by the older profile,
   without restoring the older backup;
8. Uninstall cleanup that preserves that different active original; and
9. a newer profile adopting the new original, archiving the old recovery
   generation, resuming an already copied and hash-matching archive after a
   simulated interruption, creating a fresh backup, and later restoring the
   new generation.

Validation completed on 2026-08-27:

- `./scripts/build.sh`: passed, including Bink re-export, Rosetta self-patch,
  bridge smoke probes, lifecycle trace smoke, and MoltenVK configuration probes;
- `python3 -m unittest discover -s tools -p 'test_*.py'`: 138 tests passed;
- Python bytecode compilation, all Zsh syntax checks, and `git diff --check`:
  passed; and
- `./tools/test_release_installer.sh`: passed all exact-install, update-repair,
  externally restored original preservation, and recovery-generation rotation
  cases.

No game, launcher, Steam process, installed ESO file, user setting, or pipeline
cache was modified by this source-only test.

## Interpretation

Update compatibility is now the conjunction of two independent facts: the ESO
executable remains bridge-compatible, and the original loader belongs to a
generation explicitly recognized by the installing release. A retained bridge
can establish the first fact but cannot establish the second; requiring vendor
Repair is therefore a safety boundary, not a missing convenience case.

This candidate must ship together with the measurement-stripped runtime in a
new immutable release. The published 0.1.2 asset remains unchanged and retains
its historical bridge-retained recovery behavior.

## Rollback

No rollback was required. All mutations occurred inside a disposable temporary
fixture that was removed by the test harness.

## 2026-08-27 amendment: same-target patcher upgrades

Release-candidate testing added the adjacent upgrade case without weakening the
update boundary. A newer patcher may replace or uninstall an earlier bridge
directly only when all of the following agree: current ESO SHA-256, marker
attestation, recorded executable SHA-256, recorded original-loader SHA-256, and
the verified backup. The restore occurs only after install confirmation and the
transaction mutation boundary begins. If the earlier installation applied the
settings template and the player selects settings-skip for the binary-only
upgrade, the existing conflict-safe settings restore record is retained.

The disposable fixture changes the bridge Mach-O identity to produce a distinct
earlier-patcher hash while preserving its bridge classification. It proves
same-generation Install replacement, settings-record retention, and Uninstall
of that earlier bridge. Executable-update cases still fail closed and require
launcher Repair. The complete 0.1.3 release transaction fixture passes.
