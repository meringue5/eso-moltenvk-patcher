# ESO MoltenVK Patcher release packaging

The release artifact available without an Apple Developer membership is
**ESO-MoltenVK-Patcher-<version>.zip**. It contains prebuilt payloads plus
player-facing `Install.command`, `Uninstall.command`, `Status.command`,
`Diagnostics.command`, and `README.txt`; it is an installer and maintenance
tool, not a game launcher. A signed **ESO MoltenVK Patcher.app**
in a compressed DMG remains the future polished distribution channel.

## Build a release candidate

```sh
./scripts/package-release-zip.sh 0.1.0
```

The command rebuilds the bridge, runs its non-game gates, embeds the bridge,
MoltenVK runtime, current target profile, and sanitized M4 settings template,
writes a SHA-256 manifest,
and emits a ZIP under `dist/`. Python and Xcode are release-author tooling only:
players need neither of them, nor a source checkout.

Players normally run only `Install.command`; it discovers and validates the
client, then asks for confirmation immediately before changing files.
It also presents an explicit settings-template choice with Apply highlighted
initially. In an interactive Terminal the player may use arrows or Y/N; non-interactive callers
must pass `--apply-settings` or `--skip-settings`. `--yes` accepts only the ESO
application target and never implies a settings choice.
Payloads and checksums live under the hidden `.eso-moltenvk-patcher` directory;
`Status.command` reports read-only health, `Diagnostics.command` exports a
privacy-filtered support ZIP, and `Uninstall.command` restores the original.
If Finder
does not permit a downloaded command to run directly, they can drag it into a
Terminal window or run `zsh Install.command`; this is the unsigned-release
tradeoff. Do not ask users to run a remote `curl | sh` command.

The commands render a lightweight, dependency-free terminal stepper rather
than raw implementation logs. Numbered stages correspond to completed safety
or mutation boundaries; there is no simulated time-based progress. Color is
enabled only for an interactive terminal and disabled by `NO_COLOR` or output
redirection, keeping support logs machine-readable.
The interactive renderer clears only the visible viewport, then redraws the
full plan after each completed boundary. Pending rows are grey and unmarked;
completed rows use a bright label and green checked box. Non-interactive output
never emits screen-clearing or cursor-control sequences.
Interactive target and settings confirmations are dependency-free single-key
menus with the affirmative first row highlighted initially. Arrow keys move
the highlight, Return confirms it, Y/N act as shortcuts, and Escape cancels. The script
does not alter persistent terminal input settings.

The package prints its release version on every invocation. If no known client
is found, an interactive Terminal session asks the player to drag `eso.app` or
the ESO Launcher into the window. A successful install reports the exact target,
release version, and verified backup location. The per-installation state keeps
a `.version` record for later support and survives removal alongside the
verified backup.

The same per-installation directory journals `prepared`, `installed`,
`recovered`, and `removed` transaction phases. A repeated Install after an
interruption never trusts a partial binary: it validates the exact executable,
state, backup, and active Bink identity, restores the verified original, removes
only patch-owned artifacts, and restarts from the clean baseline.

Version 0.2.0 retains the measurement-stripped
`startup-compositor-neutralize-pacing-release` profile. It retains the exact
inactive 100-ms pacing bypass and bounded 79-draw compositor repair while
disabling pipeline timing, post-window lifecycle bookkeeping, and Metal
argument buffers. It adds package-integrity verification, a versioned Balanced
M4 profile, public architecture-backed Status, privacy-filtered diagnostics,
and bounded production-log rotation without changing the runtime control. The
package
accepts its selected exact profile directly. For a different executable it
first requires the embedded MoltenVK archive hash to remain unchanged, then
runs the bundled native compatibility auditor over every compiled patch
signature, old-runtime text-boundary reference, and proc-query multiplicity.
The audited executable SHA-256 is stored in the marker and rechecked by the
runtime. The installer binds executable attestation and original-loader
identity as one recovery generation and supports a verified same-target upgrade
from an earlier patcher. A bridge retained across an executable update still
requires launcher Repair because the current vendor original cannot be proven.

## Installer behavior

- Searches the known Steam and ZeniMax launcher client locations.
- Accepts `--eso-app` with an `eso.app` or launcher location when the client is
  installed elsewhere.
- Checks the selected exact ESO executable SHA-256, or requires the complete
  packaged compatibility audit before accepting a later executable.
- Checks the matching original Bink-library SHA-256, then gives the player's
  private copied original the bridge's loader identity; no proprietary Bink
  binary is included in the release ZIP or DMG.
- Requires the selected bundle to be idle; idle Steam alone is not a blocker.
- Creates an independently verified original-Bink backup in Application
  Support, while retaining the renamed original needed by the runtime proxy.
- Exposes visible one-step Install, Uninstall, read-only Status, and private
  Diagnostics commands while retaining CLI Repair for recovery workflows.
- Optionally merges the versioned `balanced-m4-1920x1200-v1` profile's exactly
  48 allowlisted keys into a verified `UserSettings.txt` backup. Remove restores
  the backup only if the applied settings have not subsequently changed.
- Verifies checksums for every visible command and hidden payload before any
  action, and emits a local 0600 support ZIP containing only allowlisted state
  and latest-run evidence.
- Binds executable attestation and original-Bink SHA-256 as one recovery
  generation. A stale retained bridge requires launcher Repair; a different
  launcher-provided original is never overwritten by an older backup.
- Restores the original Bink library on Remove only when the active bridge,
  marker, executable, state, and backup match one generation. If the launcher
  already supplied a non-bridge loader, Remove preserves it byte-for-byte and
  deletes only exact patch-owned companions.

An unrecognized client build fails closed. This permits a shared Steam/direct
client profile when its exact executable matches, without assuming that either
path is trustworthy.

Release assembly runs a disposable end-to-end fixture covering checksum
refusal, a deliberately interrupted install and verified recovery, Status,
privacy-filtered Diagnostics, versioned settings state, Install, Remove,
reinstall, same-payload package promotion, launcher-restored update recovery,
retained-bridge repair refusal, launcher-provided-original preservation, and
supported generation rotation. A clean-extraction test verifies the exact
visible layout, executable bits, LF line endings, and every package checksum,
and rejects ZIPs containing `.DS_Store`, AppleDouble `._` entries, `__MACOSX`,
settings, caches, or crash evidence.

## Optional signed DMG

`./scripts/package-dmg.sh 0.1.0` still assembles the native app DMG for a
future signed release. It is not the unsigned public-release artifact.

## Signing and notarization gate

The local build creates an ad-hoc-signed development DMG when no signing
identity is available. A public GitHub Release must not use that artifact.

Before publication, install a Developer ID Application certificate and run:

```sh
CODESIGN_IDENTITY='Developer ID Application: Your Name (TEAMID)' \
  ./scripts/package-dmg.sh 1.0.0
```

Then submit the finished DMG using the team's notarization profile, staple the
notarization ticket, and verify Gatekeeper acceptance on a clean Apple Silicon
Mac. The identity and notarization credentials are release secrets and must
never be committed.

The repository supplies the credential-free final step:

```sh
NOTARY_PROFILE='ESO MoltenVK Patcher Notary' \
  ./scripts/notarize-dmg.sh dist/ESO-MoltenVK-Patcher-1.0.0.dmg
```

`NOTARY_PROFILE` names a local `xcrun notarytool store-credentials` keychain
profile; it is not a secret value to place in the repository or an environment
file.
