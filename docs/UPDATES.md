# ESO update runbook

Launcher-managed updates replace files after the launcher opens, so a local
tool cannot promise that no remote update is pending. The update gate instead
detects the resulting local build boundary immediately, prevents stale
evidence or installation, and automates the safe fast path when the relevant
binary layout is unchanged.

No command in this document launches ESO, Steam, or the launcher.

## 1. Run the routine fast gate

For an ordinary launcher update notification, run:

```sh
./scripts/quick-update-check.sh
```

`READY` means the exact selected ESO executable, client version, and databuild
are present, the last completed full-coverage launcher comparison says every
live repository is current, and the installed bridge target and enable marker
match. `STOP` exits `3`, names the failing component, and prints the relevant
component detail. The launcher
snapshot must be no more than one hour old by default, so freshness is enforced
by code rather than visual timestamp inspection. Override
`TESO4M4_LAUNCHER_MAX_AGE_SECONDS` only for a documented diagnostic reason.
On the 2026-07-25 M4 checkpoint, the complete quick gate took approximately
0.15–0.16 seconds on five warm runs; it is the default path.
Do not repeat manual log searches or a full static rebase audit after `READY`.

Only continue to the sections below when the gate reports `STOP`.

## 2. Detect the local target

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
| `CONTENT_UPDATE` | 3 | Executable matches, but client version/databuild changed |
| `UNKNOWN_UPDATE` | 3 | No checked-in target matches |

`scripts/prepare-evidence.sh` runs the exact and combined gates automatically
and refuses to prepare a stale experiment. Evidence preparation and collection
also preserve the full launcher repository IDs, settings hashes, and controlled
setting values. An update during the launcher boundary therefore remains
machine-checkable without a manual log search.

## 3. Classify a launcher-reported update

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

When the launcher reports an update but the quick gate returns `READY`, the
executable, client build, remote repository state, and bridge checkpoint have
already been checked in code. Preserve raw updater logs only when the event
itself is research evidence; routine analysis stops at `READY`.

## 4. Audit the unchanged-layout fast path

For an unknown local build, choose a new dated manifest name and an accurate
description:

```sh
./scripts/rebase-update.sh \
  config/targets-eso-YYYY-MM-DD.json \
  'Steam macOS ESO VERSION build analyzed on YYYY-MM-DD'
```

The audit accepts the fast path only when all semantic bridge boundaries match
the selected reference profile:

- both object members of the embedded MoltenVK archive;
- the main MoltenVK object hash, 162 Vulkan text symbols, and link delta;
- all 17 target symbol offsets and their exact 12-byte patch signatures;
- the complete external-reference count grouped by target and reference kind;
- the GIPA and GDPA routes, direct-query counts, recovered-name multiplicities,
  and zero unnamed sites;
- the pinned replacement-runtime hash, export count, and every externally
  referenced Vulkan export.

Source addresses may move as the surrounding ESO code changes; address values
alone are not a compatibility boundary. Patch bytes, target semantics,
reference kinds/counts, routes, and recovered names remain strict. The
executable, archive, and replacement runtime are hashed again before the
result is accepted, which rejects files changing during a launcher update.
Any mismatch returns `MANUAL_ANALYSIS_REQUIRED`, writes no candidate, and does
not change the selected target.

The audit also reads the client version/databuild before analysis, rechecks the
stamp afterward, and writes that identity into the candidate manifest. On the
2026-07-25 checkpoint, this full static path took 53.8 seconds and reproduced
the exact profile without modifying the game bundle. It is a coded `STOP` path,
not work performed after a routine `READY`.

On success, the script writes the candidate manifest and updates
`config/current-target.txt`. It does not restore, patch, or install anything in
the game bundle. Review and commit both source changes before continuing.

## 5. Rebuild and install through the normal gate

A successful fast audit establishes only static equivalence. With ESO, Steam,
and the launcher stopped, the existing guarded sequence still applies:

1. Verify the current status and required restore path.
2. Restore the pristine loader only when needed for a clean source rebuild.
3. Fetch the pinned MoltenVK release and rebuild from source.
4. Run the full non-game probes and static checks.
5. Prepare fresh evidence.
6. Install only under the explicit source-tool installation gate.
7. Ask the user for the bounded Steam-path runtime test.

The public 0.1.1 and later packages carry a native form of the same fail-closed boundary.
After a launcher update, re-running its `Install.command` first requires an
unchanged embedded MoltenVK archive, then validates all compiled patch bytes,
old-runtime text references, and proc-query multiplicities. If those match, it
records the new executable SHA-256 in the marker; the bridge independently
checks that attestation before changing memory. The next release also binds the
restore backup to its executable/original-loader generation. A retained stale
bridge requires the ESO launcher Repair path because the current vendor
original cannot be observed safely; the old backup is not restored. A
launcher-restored original is reused only when its hash matches the release
profile. A newer supported original generation rotates the old recovery pair
into history before receiving a fresh backup.

This packaged path avoids a new release for relocation-only ESO updates. It
does not prove lobby or world rendering and must stop when the archive,
reference shape, proc route, or patch signature changes. Such a change requires
manual analysis rather than a weakened profile.
