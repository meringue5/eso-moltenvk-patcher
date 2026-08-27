ESO MoltenVK Patcher replaces ESO's embedded legacy MoltenVK path on a verified macOS client.
It installs beside the selected eso.app; it is not an AddOn and does not use Documents/Elder Scrolls Online/live/AddOns.
If anything goes wrong, run Uninstall.command to restore the verified original Bink library.

QUICK START

1. Quit ESO and the ZeniMax launcher. Steam may remain open if ESO is not updating.
2. Double-click Install.command. It finds and verifies ESO, then asks before changing files.
   You must also explicitly choose y or n when asked whether to apply the
   validated M4 2048 x 1280 settings template. There is no default choice.
3. To uninstall, double-click Uninstall.command.

Version 0.1.3 is the strongest startup-stability release so far for the
validated M4 and ESO 12.0.8 target. It prevents ESO's stale inactive 100-ms
pacing path, suppresses only the verified bounded pink-placeholder window,
and retires diagnostic timing and lifecycle bookkeeping after startup. This
does not promise identical FPS on every Mac, but it directly addresses both
observed sides of the intermittent pink-and-low-FPS startup condition.

Support diagnostics and payload files are kept in a hidden internal folder;
they are not separate installation steps. Keep this folder together and do not
move Install.command or Uninstall.command out of it.

The scripts look in the known Steam and ZeniMax locations. If ESO is elsewhere,
the Terminal asks you to drag eso.app or the ESO Launcher.app into the window.
You can also run a command explicitly:

  ./Install.command --eso-app '/path/to/eso.app'

If macOS blocks a downloaded command, open System Settings > Privacy & Security,
scroll down, and choose Open Anyway. Removing quarantine attributes is not the
normal installation procedure.

The scripts never launch ESO, Steam, or the launcher. An exact supported client
is accepted directly. A later game update is accepted only when the bundled
auditor proves that the embedded MoltenVK, patch bytes, reference boundary, and
proc-query routes remain compatible. A changed runtime or layout, modified
original library, running game/launcher, active Steam update, or indeterminate
bundle state stops without changing files.

After the ESO launcher updates or repairs the client, quit ESO and the launcher
and run Status. If the same supported original loader is active, run
Install.command again. If the bridge remains active with a stale attestation,
run the ESO launcher's Repair first; Install and Uninstall will not restore the
previous backup across an executable update. A different launcher-provided
original loader is preserved and requires a newer compatible patcher release.

If you choose settings application, only the template's 48 allowlisted keys
are merged into UserSettings.txt. The complete file is never replaced with a
generic copy. The original is backed up. Remove restores it only if the
settings still match the applied result; later user changes are never silently
overwritten.

Installation progress is journaled in the per-installation Application Support
folder. If an install was interrupted, running Install.command again verifies
the journal and backup, restores the clean baseline, and safely restarts. It
never resumes from an unverified partially copied binary.

Source, supported builds, and troubleshooting:
https://github.com/meringue5/eso-moltenvk-patcher

Latest release:
https://github.com/meringue5/eso-moltenvk-patcher/releases/latest
