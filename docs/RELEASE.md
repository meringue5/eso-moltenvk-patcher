# ESO MoltenVK Patcher release packaging

The public artifact is **ESO MoltenVK Patcher.app** in a compressed DMG. It is
an installer and maintenance tool, not a game launcher. It locates a Steam or
ZeniMax ESO client, or lets the player select one, then patches only an exact
supported executable profile.

## Build a release candidate

```sh
./scripts/package-dmg.sh 0.1.0
```

The command rebuilds the bridge, runs its non-game gates, compiles the native
macOS installer app, embeds only the bridge, MoltenVK runtime, and current
target profile, verifies the app signature, and emits a DMG under `dist/`.
Neither Python nor a source checkout is required by players who use the DMG.

## Installer behavior

- Searches the known Steam and ZeniMax launcher client locations.
- Accepts a player-selected `eso.app` or launcher location.
- Checks the exact ESO executable SHA-256 before any file change.
- Checks the matching original Bink-library SHA-256, then gives the player's
  private copied original the bridge's loader identity; no proprietary Bink
  binary is included in the release DMG.
- Requires the selected bundle to be idle; idle Steam alone is not a blocker.
- Creates an independently verified original-Bink backup in Application
  Support, while retaining the renamed original needed by the runtime proxy.
- Exposes Check, Install, Repair, and Remove.
- Restores the original Bink library on Remove only after the saved restore
  record, backup hash, and selected executable still match the exact profile;
  it does not launch ESO or alter account authentication.

An unrecognized client build fails closed. This permits a shared Steam/direct
client profile when its exact executable matches, without assuming that either
path is trustworthy.

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
