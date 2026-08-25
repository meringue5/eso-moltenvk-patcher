# ESO MoltenVK Patcher release packaging

The release artifact available without an Apple Developer membership is
**ESO-MoltenVK-Patcher-<version>.zip**. It contains prebuilt payloads plus
player-facing `Install.command`, `Uninstall.command`, and `README.txt`; it is an installer
and maintenance tool, not a game launcher. A signed **ESO MoltenVK Patcher.app**
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
Payloads, checksums, and optional Status diagnostics live under the hidden
`.eso-moltenvk-patcher` directory; `Uninstall.command` restores the original.
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

Version 0.1.2 retains repeated Install support after a compatible game update
and selects the performance-first `startup-pipeline-timing-control` profile.
The original pink startup placeholder may remain visible; the compositor
neutralizer is deliberately excluded because low-FPS avoidance has priority.
The package
accepts its selected exact profile directly. For a different executable it
first requires the embedded MoltenVK archive hash to remain unchanged, then
runs the bundled native compatibility auditor over every compiled patch
signature, old-runtime text-boundary reference, and proc-query multiplicity.
The audited executable SHA-256 is stored in the marker and rechecked by the
runtime. Launcher-restored-original and bridge-retained update states both
restore from the verified backup before reinstalling.

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
- Exposes one-step Install and Uninstall while retaining hidden Status and CLI
  Repair diagnostics.
- Optionally merges exactly 48 allowlisted M4-profile keys into a verified
  `UserSettings.txt` backup. Remove restores the backup only if the applied
  settings have not subsequently changed.
- Restores the original Bink library on Remove only after the saved restore
  record, backup hash, and selected executable still match the exact profile;
  it does not launch ESO or alter account authentication.

An unrecognized client build fails closed. This permits a shared Steam/direct
client profile when its exact executable matches, without assuming that either
path is trustworthy.

Release assembly runs a disposable end-to-end fixture covering a deliberately
interrupted install and verified recovery, Status, Install, Remove, reinstall,
launcher-restored update recovery, and bridge-retained update recovery;
verifies LF line endings and every payload checksum; and rejects
ZIPs containing `.DS_Store`, AppleDouble `._` entries, or `__MACOSX`.

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
