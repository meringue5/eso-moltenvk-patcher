#!/bin/zsh
set -euo pipefail

if [[ "${TESO4M4_EXPERIMENTAL:-}" != "I_ACCEPT_CRASH_RISK" ]]; then
  echo "Prior bridge experiments crashed after activation."
  echo "The current descriptor compatibility build is still unvalidated in ESO."
  echo "Read docs/STATUS.md and the linked experiment record first."
  echo "Set TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK only for a controlled test."
  exit 1
fi

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
MODE="${TESO4M4_MODE:-descriptor-compat}"
[[ "$MODE" == "descriptor-compat" || "$MODE" == "legacy-allocation" \
  || "$MODE" == "reset-resource-trace" \
  || "$MODE" == "no-command-pooling" \
  || "$MODE" == "render-audit" \
  || "$MODE" == "reset-no-pipeline-cache" \
  || "$MODE" == "full-lifetime-audit" \
  || "$MODE" == "texture-cache-fix" \
  || "$MODE" == "legacy-feature-profile" \
  || "$MODE" == "performance-safe" ]] || {
  echo "Unsupported experiment mode: $MODE"
  exit 1
}
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
PRISTINE="$GAME_MAC/libBink2Macx64.teso4m4-pristine.dylib"
ORIGINAL="$GAME_MAC/libBink2Macx64.teso4m4-original.dylib"
MVK="$GAME_MAC/libMoltenVK.teso4m4.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
PRESERVE_CACHE_STATE="${TESO4M4_PRESERVE_CACHE_STATE:-}"
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"

if pgrep -x eso >/dev/null 2>&1 \
  || pgrep -f '/ZeniMax Online Studios Launcher' >/dev/null 2>&1 \
  || pgrep -f '/Steam/Contents/MacOS/steam_osx' >/dev/null 2>&1; then
  echo "ESO, Steam, or the launcher is running. Exit all three before installation."
  exit 1
fi
if [[ -n "$PRESERVE_CACHE_STATE" && "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Unsupported TESO4M4_PRESERVE_CACHE_STATE value."
  exit 1
fi
actual_sha="$(shasum -a 256 "$ESO" | awk '{print $1}')"
[[ "$actual_sha" == "$EXPECTED_SHA" ]] || { echo "Unknown ESO build; refusing."; exit 1; }
for file in libBink2Macx64.dylib libBink2Macx64.teso4m4-original.dylib libMoltenVK.teso4m4.dylib; do
  [[ -f "$ROOT/build/$file" ]] || { echo "Run scripts/build.sh first."; exit 1; }
done

if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  echo "Active Bink is already a bridge; restore before installing."
  exit 1
fi
[[ ! -e "$MARKER" ]] || {
  echo "Enable marker already exists; restore and re-check status first."
  exit 1
}
if [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  [[ -f "$PIPELINE_CACHE" && -f "$OLD_PIPELINE_CACHE" ]] || {
    echo "Cache-preserving rebase requires both active and old-backup caches."
    exit 1
  }
else
  [[ ! -e "$OLD_PIPELINE_CACHE" ]] || {
    echo "Old pipeline-cache backup already exists; restore first."
    exit 1
  }
fi
if [[ ! -f "$PRISTINE" ]]; then
  cp -p "$BINK" "$PRISTINE"
fi
cmp -s "$BINK" "$PRISTINE" || {
  echo "Active Bink does not match the pristine restore source; refusing."
  exit 1
}
cp -p "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" "$ORIGINAL.installing"
mv -f "$ORIGINAL.installing" "$ORIGINAL"
cp -p "$ROOT/build/libMoltenVK.teso4m4.dylib" "$MVK.installing"
mv -f "$MVK.installing" "$MVK"
cp -p "$ROOT/build/libBink2Macx64.dylib" "$BINK.installing"
mv -f "$BINK.installing" "$BINK"

printf '%s\n' "$MODE" > "$MARKER"
if [[ "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" \
  && -f "$PIPELINE_CACHE" && ! -e "$OLD_PIPELINE_CACHE" ]]; then
  mv "$PIPELINE_CACHE" "$OLD_PIPELINE_CACHE"
fi
if [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Installed experimental teso4m4 bridge in $MODE mode; preserved both pipeline caches in place."
else
  echo "Installed experimental teso4m4 bridge in $MODE mode."
fi
