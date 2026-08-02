# Install ESO MoltenVK Patcher

[![Download the latest ESO MoltenVK Patcher release](https://img.shields.io/badge/Download-Latest%20Release-2ea44f?style=for-the-badge&logo=github)](https://github.com/meringue5/eso-moltenvk-patcher/releases/latest)

This guide is for the prebuilt GitHub Release ZIP. It does not require Python,
Xcode, Homebrew, or a source checkout. The illustrations below intentionally
omit personal paths; the exact macOS wording and appearance can vary by
version.

## 1. Download and open the package

Open the [latest release](https://github.com/meringue5/eso-moltenvk-patcher/releases/latest),
expand **Assets** if necessary, and download
`ESO-MoltenVK-Patcher-<version>.zip`. Do not download GitHub's automatically
generated **Source code** archives: they are for contributors and do not
contain the prebuilt release payload.

![Release Assets example with the prebuilt ZIP highlighted](images/install/01-download.svg)

Unzip the download. It opens as one folder containing three commands and its
payload. `install.command` is the only command required for installation;
`status.command` is optional.

![Package folder showing Install, Remove, and optional Status](images/install/02-package.svg)

## 2. Quit the game, then run Install

Quit ESO and the ZeniMax launcher before installation. Steam itself may remain
open unless it is downloading, updating, or holding files in the selected ESO
bundle. Double-click `install.command`.

The installer searches known Steam and official ZeniMax locations. If ESO is
elsewhere, Terminal asks for a path: drag `eso.app` or `ESO Launcher.app` from
Finder into the Terminal window and press Return. This supports renamed,
moved, and non-Steam installations without guessing their locations.

Review the displayed target. Type `y` only when it names the ESO installation
you intend to patch. The installer then verifies the exact game build,
original library, payload checksums, idle state, and recoverable backup.

The installer also requires an explicit `y` or `n` response for the bundled
M4 2048×1280 settings template. Pressing Return alone does not choose a
default; the question repeats. Choosing `y` backs up `UserSettings.txt` and
selectively merges only the 48 allowlisted keys. Choosing `n` leaves all game
settings unchanged.

![Terminal confirmation example showing the verified target and one confirmation](images/install/03-confirm.svg)

An unsupported or updated ESO build stops without changing files. Do not work
around that check; wait for a compatible patcher release.

## 3. If macOS blocks the command

An unsigned GitHub download may be blocked on first open. Try opening
`install.command` once, then go to **System Settings → Privacy & Security**,
scroll to the Security section, and choose **Open Anyway** for the blocked
item. Confirm macOS's prompt, then run Install again.

![Illustrated Privacy & Security Open Anyway location](images/install/04-open-anyway.svg)

The wording can differ by macOS version. Removing quarantine attributes with
`xattr` is not the normal installation path and is not recommended here.

## 4. Launch and uninstall

After a successful summary, close Terminal and start ESO through the same
Steam or official ESO launcher you normally use. The patcher does not replace
authentication and does not need to remain open.

To uninstall, quit ESO and the launcher and double-click `remove.command`. It
verifies and restores the recorded original library. Keep the patcher folder
until removal is complete. `status.command` reports the installed version and
state when troubleshooting, but installation never requires a separate Check
step.

If the settings template was applied, Remove restores the pre-install settings
only when the current file still exactly matches the applied result. If you
changed settings afterward, Remove preserves those changes and reports the
retained backup path instead of silently overwriting them.

If installation was interrupted, run `install.command` again. It does not
blindly continue copying from the last line: it verifies the journal and
backup, restores a clean baseline, and restarts the transaction safely.

For technical release details and supported-build policy, see
[Release packaging](RELEASE.md) and [Production baseline](PRODUCTION.md).
