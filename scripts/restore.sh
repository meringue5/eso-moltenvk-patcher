#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
PRISTINE="$GAME_MAC/libBink2Macx64.teso4m4-pristine.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
MVK_141_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-mvk-1.4.1-backup"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
PRESERVE_CACHE_STATE="${TESO4M4_PRESERVE_CACHE_STATE:-}"
STAMP="$(date +%Y%m%d-%H%M%S)"

if pgrep -x eso >/dev/null 2>&1 \
  || pgrep -f '/ZeniMax Online Studios Launcher' >/dev/null 2>&1 \
  || pgrep -f '/Steam/Contents/MacOS/steam_osx' >/dev/null 2>&1; then
  echo "ESO, Steam, or the launcher is running. Exit all three before restoring."
  exit 1
fi
[[ -f "$PRISTINE" ]] || { echo "Pristine Bink backup is missing."; exit 1; }
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"
ACTUAL_SHA="$(shasum -a 256 "$ESO" | awk '{print $1}')"
[[ "$ACTUAL_SHA" == "$EXPECTED_SHA" ]] || { echo "Unknown ESO build; refusing."; exit 1; }
if [[ -n "$PRESERVE_CACHE_STATE" && "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Unsupported TESO4M4_PRESERVE_CACHE_STATE value."
  exit 1
fi
if [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  [[ -f "$OLD_PIPELINE_CACHE" ]] || {
    echo "Cache-preserving restore requires the old-backup cache."
    exit 1
  }
  [[ -f "$PIPELINE_CACHE" || -f "$MVK_141_PIPELINE_CACHE" ]] || {
    echo "Cache-preserving restore requires an active or versioned 1.4.1 cache."
    exit 1
  }
fi

cp -p "$PRISTINE" "$BINK.restoring"
mv -f "$BINK.restoring" "$BINK"
if [[ -f "$MARKER" ]]; then
  mv "$MARKER" "$MARKER.disabled-$STAMP"
fi
if [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Restored the pristine ESO loader; preserved all pipeline-cache states in place."
else
  if [[ -f "$PIPELINE_CACHE" && -f "$OLD_PIPELINE_CACHE" ]]; then
    mv "$PIPELINE_CACHE" "${PIPELINE_CACHE}.teso4m4-new-$STAMP"
  fi
  if [[ -f "$OLD_PIPELINE_CACHE" && ! -e "$PIPELINE_CACHE" ]]; then
    mv "$OLD_PIPELINE_CACHE" "$PIPELINE_CACHE"
  fi
  echo "Restored the pristine ESO loader and old pipeline cache."
fi
