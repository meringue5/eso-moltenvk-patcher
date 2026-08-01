# ESO macOS client distribution and MoltenVK scope

Date: 2026-08-01

## Question

Does the stale MoltenVK runtime belong only to Steam's macOS package, or to the
ESO macOS game client maintained by the ZeniMax launcher? This distinction
determines whether `teso4m4` should describe itself as a Steam-only patch or as
a patch for ESO on macOS with a currently Steam-validated installer.

## Primary-source record

- ZeniMax's [PC/Mac v4.2.5 patch notes](https://forums.elderscrollsonline.com/en/discussion/comment/5552032/)
  state that Update 20 switched the Mac renderer from OpenGL to MoltenVK in
  October 2018. The announcement applies to the Mac game client, not to a
  Steam-specific renderer.
- Khronos's [MoltenVK release history](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/Whats_New.md#moltenvk-1018)
  dates MoltenVK 1.0.18 to 2018-08-15. The current locally fingerprinted ESO
  12.0.7 macOS executable still contains that version as static code; see
  [Durable findings](../FINDINGS.md#runtime-architecture).
- ZeniMax support says [Steam still opens the ESO launcher](https://help.elderscrollsonline.com/app/answers/detail/a_id/21674/~/do-i-still-need-a-launcher-to-play-eso-on-steam)
  rather than acting as the complete game updater by itself.
- ZeniMax's Mac recovery instructions distinguish how the launcher is obtained:
  the account page for direct owners and the Steam library for Steam owners.
  They explicitly say that only the launcher is reinstalled in the direct
  path; see [Why can't I launch ESO on the latest macOS?](https://help.elderscrollsonline.com/app/answers/detail/a_id/48594/~/why-cant-i-launch-eso-on-the-lastest-macos).
- ZeniMax also documents a recovery procedure that copies the Steam-installed
  `The Elder Scrolls Online` game folder into a website-launcher installation
  and then resumes through that launcher; see
  [Steam Mac Play troubleshooting](https://help.elderscrollsonline.com/app/answers/detail/a_id/26040/~/what-do-i-do-if-i-click-play-and-nothing-happens-on-steam-when-playing-on-mac).
  That is strong evidence of a shared Mac client/content format rather than a
  deliberately frozen Steam-only game branch.
- Apple Silicon remains outside official ESO support according to
  [ZeniMax's ARM-based Mac article](https://help.elderscrollsonline.com/app/answers/detail/a_id/52307/~/can-i-run-eso-on-an-arm-based-mac).

## Local distribution evidence

The current Steam installation is not merely the original Steam depot left
untouched. The ZeniMax launcher recorded a completed `Live_Prod` update check
for eight repositories, including `MacPubPlayerClient` and `public_depot`, and
reported identical local and remote repository identities. The exact current
game target is ESO 12.0.7, databuild `3281538`, and passes the repository's
fingerprint gate.

The project scripts currently default to the Steam installation path. They can
accept an explicit `ESO_APP` path, but the exact-build manifest, installation
workflow, authentication path, and gameplay validation all come from the
Steam installation.

## Interpretation

The stale runtime is best described as an **ESO macOS client** issue, not as
evidence that Steam distributes a separate obsolete game executable. The
current client still embeds a MoltenVK release from August 2018 even though the
Mac renderer and live game content continue to receive launcher-managed
updates.

The historical evidence does not prove that ZeniMax never tested or briefly
shipped another MoltenVK revision in an intermediate build. It does establish
the important present fact: the current 2026 ESO 12.0.7 macOS client under test
executes statically linked MoltenVK 1.0.18.

The public project should therefore use two separate scope statements:

1. **Technical target:** the exact ESO macOS `eso.app` executable and its
   statically embedded MoltenVK runtime.
2. **Currently supported and validated distribution:** the Steam macOS
   installation and its normal Steam-authenticated launcher path.

Do not claim direct-launcher support until a current non-Steam `eso.app` is
fingerprinted and shown to match a supported target or receives its own exact
manifest and installation-path validation.

## Search limitation

Official patch notes announce the 2018 renderer transition but do not publish
a per-build MoltenVK version history. No direct-install 2026 binary was
available for a byte-for-byte comparison. The shared-client conclusion is
therefore supported by official launcher recovery procedures and the local
launcher repository model, while direct-install compatibility remains
unverified.
