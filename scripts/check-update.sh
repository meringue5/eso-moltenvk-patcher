#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
ESO="$ESO_APP/Contents/MacOS/eso"

exec python3 "$ROOT/tools/eso_update.py" check \
  --exe "$ESO" \
  --manifest-dir "$ROOT/config" \
  --current-manifest "$MANIFEST"
