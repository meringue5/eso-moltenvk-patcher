# ESO update runbook

Launcher-managed updates replace files after the launcher opens, so a local
tool cannot promise that no remote update is pending. The update gate instead
detects the resulting local build boundary immediately, prevents stale
evidence or installation, and automates the safe fast path when the relevant
binary layout is unchanged.

Neither command in this document launches ESO, Steam, or the launcher.

## 1. Detect the local target

Run:

```sh
./scripts/check-update.sh
```

The checker hashes the local executable, reads its Mach-O UUID, scans every
checked-in `targets-eso-*.json`, and compares the result with
`config/current-target.txt`.

| Result | Exit | Meaning |
|---|---:|---|
| `CURRENT` | 0 | Exact selected executable and UUID |
| `KNOWN_OTHER` | 3 | Historical target, not the selected target |
| `IDENTITY_MISMATCH` | 3 | Hash record and UUID disagree; investigate |
| `UNKNOWN_UPDATE` | 3 | No checked-in target matches |

`scripts/prepare-evidence.sh` now runs this gate automatically and refuses to
prepare a stale experiment. Evidence collection records the post-run result as
well, so an update during the launcher boundary remains visible.

## 2. Classify a launcher-reported update

The executable checker intentionally answers only whether the installed bridge
still targets the exact ESO binary. A launcher self-update can therefore leave
that result at `CURRENT`. After the launcher has completed its own state check,
run:

```sh
./scripts/check-launcher-state.sh
```

The command reads the newest local launcher host log without launching an app
or contacting the network. It accepts only a completed, full-coverage
`Live_Prod` `stateCheck`, requires the launcher's `noUpdateRequired` path, and
verifies that every recorded repository has identical local and remote IDs.
Later two-repository lightweight checks do not replace the last full snapshot.
`CURRENT_REMOTE` is a point-in-time result from that launcher session, not a
promise about a future remote update. `UPDATE_OR_ERROR` or `INDETERMINATE`
blocks a prepared experiment until the event is understood.

When the launcher reports an update but both checks return `CURRENT` and
`CURRENT_REMOTE`, preserve the self-update and host logs, then compare the game
databuild stamp and bundle modification boundary. If those are unchanged, the
event may be recorded as a launcher-only update and does not require an ESO
target rebase.

## 3. Audit the unchanged-layout fast path

For an unknown local build, choose a new dated manifest name and an accurate
description:

```sh
./scripts/rebase-update.sh \
  config/targets-eso-YYYY-MM-DD.json \
  'Steam macOS ESO VERSION build analyzed on YYYY-MM-DD'
```

The audit accepts the fast path only when all of these are exact matches with
the selected reference profile:

- both object members of the embedded MoltenVK archive;
- the main MoltenVK object hash, 162 Vulkan text symbols, and link delta;
- all 17 target symbol offsets and their exact 12-byte patch signatures;
- all 40 external-reference source sites, grouped by target and reference kind;
- the GIPA and GDPA slot offsets, all 19 and 80 direct query source sites, their
  recovered names, and zero unnamed sites;
- the pinned replacement-runtime hash, export count, and every externally
  referenced Vulkan export.

The executable, archive, and replacement runtime are hashed again before the
result is accepted, which rejects files changing during a launcher update.
Any mismatch returns `MANUAL_ANALYSIS_REQUIRED`, writes no candidate, and does
not change the selected target.

On success, the script writes the candidate manifest and updates
`config/current-target.txt`. It does not restore, patch, or install anything in
the game bundle. Review and commit both source changes before continuing.

## 4. Rebuild and install through the normal gate

A successful fast audit establishes only static equivalence. With ESO, Steam,
and the launcher stopped, the existing guarded sequence still applies:

1. Verify the current status and required restore path.
2. Restore the pristine loader only when needed for a clean source rebuild.
3. Fetch the pinned MoltenVK release and rebuild from source.
4. Run the full non-game probes and static checks.
5. Prepare fresh evidence.
6. Install only under the explicit experimental installation gate.
7. Ask the user for the bounded Steam-path runtime test.

The fast path does not prove lobby or world rendering and cannot authorize a
game-bundle modification. If the archive, reference shape, proc route, or patch
signature changes, perform a new manual analysis instead of weakening the
profile.
