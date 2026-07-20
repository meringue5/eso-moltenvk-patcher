#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
REFERENCE="$(teso4m4_resolve_target_manifest "$ROOT")"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
ESO="$ESO_APP/Contents/MacOS/eso"
ARCHIVE="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"
NEW_RUNTIME="${MVK_ROOT:-$ROOT/vendor/MoltenVK}/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
OUTPUT="${1:-}"
DESCRIPTION="${2:-}"

[[ -n "$OUTPUT" && -n "$DESCRIPTION" ]] || {
  echo "usage: $0 config/targets-eso-YYYY-MM-DD.json 'target description'"
  exit 2
}
OUTPUT="${OUTPUT:A}"
[[ "$OUTPUT:h" == "$ROOT/config" && "$OUTPUT:t" == targets-eso-*.json ]] || {
  echo "Output must be config/targets-eso-*.json inside this repository."
  exit 2
}

python3 "$ROOT/tools/eso_update.py" audit \
  --exe "$ESO" \
  --archive "$ARCHIVE" \
  --reference "$REFERENCE" \
  --new-runtime "$NEW_RUNTIME" \
  --description "$DESCRIPTION" \
  --output "$OUTPUT"

python3 "$ROOT/tools/eso_update.py" select \
  --exe "$ESO" \
  --candidate "$OUTPUT" \
  --pointer "$ROOT/config/current-target.txt"

echo "Fast rebase manifest selected. The game bundle was not modified."
echo "Review and commit the manifest before restore, rebuild, and installation."
