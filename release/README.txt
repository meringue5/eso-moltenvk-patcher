ESO MoltenVK Patcher replaces ESO's embedded legacy MoltenVK path on one exact verified macOS client.
It installs beside the selected eso.app; it is not an AddOn and does not use Documents/Elder Scrolls Online/live/AddOns.
If anything goes wrong, run remove.command to restore the verified original Bink library.

QUICK START

1. Quit ESO and the ZeniMax launcher. Steam may remain open if ESO is not updating.
2. Double-click install.command. It finds and verifies ESO, then asks once before changing files.
3. To uninstall, double-click remove.command.

status.command is optional. Use it only when you want to check the installed
version or diagnose a support problem; it is not a required installation step.

The scripts look in the known Steam and ZeniMax locations. If ESO is elsewhere,
the Terminal asks you to drag eso.app or the ESO Launcher.app into the window.
You can also run a command explicitly:

  ./install.command --eso-app '/path/to/eso.app'

If macOS blocks a downloaded command, open System Settings > Privacy & Security,
scroll down, and choose Open Anyway. Removing quarantine attributes is not the
normal installation procedure.

The scripts never launch ESO, Steam, or the launcher. An unknown client build,
modified original library, running game/launcher, active Steam update, or
indeterminate bundle state stops without changing files.

Installation progress is journaled in the per-installation Application Support
folder. If an install was interrupted, running install.command again verifies
the journal and backup, restores the clean baseline, and safely restarts. It
never resumes from an unverified partially copied binary.

Source, supported builds, and troubleshooting:
https://github.com/lvcwoo/teso4m4
