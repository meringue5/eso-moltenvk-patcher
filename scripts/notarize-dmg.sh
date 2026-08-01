#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
DMG="${1:?usage: scripts/notarize-dmg.sh path/to/ESO-MoltenVK-Patcher.dmg}"
PROFILE="${NOTARY_PROFILE:?Set NOTARY_PROFILE to a preconfigured notarytool keychain profile.}"

[[ -f "$DMG" ]] || {
  echo "DMG does not exist: $DMG" >&2
  exit 1
}

xcrun notarytool submit "$DMG" --keychain-profile "$PROFILE" --wait
xcrun stapler staple "$DMG"
xcrun stapler validate "$DMG"
spctl -a -vv -t open "$DMG"
echo "Notarized and stapled: ${DMG:A}"
