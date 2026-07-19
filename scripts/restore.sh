#!/bin/zsh
set -euo pipefail

ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
BINK="$GAME_MAC/libBink2Macx64.dylib"
PRISTINE="$GAME_MAC/libBink2Macx64.teso4m4-pristine.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
PIPELINE_CACHE="$HOME/Documents/Elder Scrolls Online/live/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
STAMP="$(date +%Y%m%d-%H%M%S)"

if pgrep -x eso >/dev/null 2>&1; then
  echo "ESO is running. Exit it before restoring."
  exit 1
fi
[[ -f "$PRISTINE" ]] || { echo "Pristine Bink backup is missing."; exit 1; }

cp -p "$PRISTINE" "$BINK.restoring"
mv -f "$BINK.restoring" "$BINK"
if [[ -f "$MARKER" ]]; then
  mv "$MARKER" "$MARKER.disabled-$STAMP"
fi
if [[ -f "$PIPELINE_CACHE" && -f "$OLD_PIPELINE_CACHE" ]]; then
  mv "$PIPELINE_CACHE" "${PIPELINE_CACHE}.teso4m4-new-$STAMP"
fi
if [[ -f "$OLD_PIPELINE_CACHE" && ! -e "$PIPELINE_CACHE" ]]; then
  mv "$OLD_PIPELINE_CACHE" "$PIPELINE_CACHE"
fi
echo "Restored the pristine ESO loader and old pipeline cache."

