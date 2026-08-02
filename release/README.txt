ESO MoltenVK Patcher replaces ESO's embedded legacy MoltenVK path on one exact verified macOS client.
It installs beside the selected eso.app; it is not an AddOn and does not use Documents/Elder Scrolls Online/live/AddOns.
If anything goes wrong, run remove.command to restore the verified original Bink library.

QUICK START

1. Quit ESO and the ZeniMax launcher. Steam may remain open if ESO is not updating.
2. Double-click check.command, then install.command.
3. To uninstall, double-click remove.command.

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

Source, supported builds, and troubleshooting:
https://github.com/lvcwoo/teso4m4
