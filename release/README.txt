ESO MoltenVK Patcher replaces ESO's embedded legacy MoltenVK path on a verified macOS client.
It installs beside the selected eso.app; it is not an AddOn and does not use Documents/Elder Scrolls Online/live/AddOns.
If anything goes wrong, run Uninstall.command to restore the verified original Bink library.

QUICK START

1. Quit ESO and the ZeniMax launcher. Steam may remain open if ESO is not updating.
2. Double-click Install.command. It finds and verifies ESO, then asks before changing files.
   You must also explicitly choose y or n when asked whether to apply the
   balanced M4 1920 x 1200 settings template. There is no default choice.
3. Double-click Status.command at any time to verify the client, bridge,
   recovery generation, settings profile, and the latest bounded startup run.
4. Diagnostics.command writes a privacy-filtered support ZIP to your Desktop.
   It never includes UserSettings.txt, caches, credentials, or game files.
5. Logging.command selects normal info logging or temporary debug incident
   capture for the next ESO launch. It changes no bridge, cache, or game setting.
6. To uninstall, double-click Uninstall.command.

Version 0.2.1 supports the 2026-09-30 ESO update (client 12.1.5, databuild
3303624). That update relinked the game and moved the unchanged embedded
MoltenVK, so the 0.2.0 package correctly refused it. 0.2.1 keeps the 0.2.0
runtime control and settings template unchanged, verifies the new executable
and its new original Bink library, and adds a few timestamped, bounded
activation records to the local log. It does not promise identical FPS on
every Mac or claim unmeasured Quality/Efficiency profiles.

Known issue: on some launches macOS does not make ESO the active application
when the launcher starts it. ESO then runs at reduced FPS, and the mouse may
not be captured, until you switch to another app and back (Cmd-Tab). This
happens with or without the patch and is not fixed by 0.2.1.

Payload files are kept in a hidden internal folder. Keep the entire package
together and do not move any command out of it.

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
