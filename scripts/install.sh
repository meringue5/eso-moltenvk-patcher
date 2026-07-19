#!/bin/zsh
set -euo pipefail

if [[ "${TESO4M4_EXPERIMENTAL:-}" != "I_ACCEPT_CRASH_RISK" ]]; then
  echo "The current bridge is known to crash after activation."
  echo "Read docs/STATUS.md and the linked experiment record first."
  echo "Set TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK only for a controlled test."
  exit 1
fi

ROOT="${0:A:h:h}"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
PRISTINE="$GAME_MAC/libBink2Macx64.teso4m4-pristine.dylib"
ORIGINAL="$GAME_MAC/libBink2Macx64.teso4m4-original.dylib"
MVK="$GAME_MAC/libMoltenVK.teso4m4.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
PIPELINE_CACHE="$HOME/Documents/Elder Scrolls Online/live/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
MANIFEST="${TESO4M4_TARGET_MANIFEST:-$ROOT/config/targets-eso-2026-07-11.json}"
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"

if pgrep -x eso >/dev/null 2>&1; then
  echo "ESO is running. Exit it before installation."
  exit 1
fi
actual_sha="$(shasum -a 256 "$ESO" | awk '{print $1}')"
[[ "$actual_sha" == "$EXPECTED_SHA" ]] || { echo "Unknown ESO build; refusing."; exit 1; }
for file in libBink2Macx64.dylib libBink2Macx64.teso4m4-original.dylib libMoltenVK.teso4m4.dylib; do
  [[ -f "$ROOT/build/$file" ]] || { echo "Run scripts/build.sh first."; exit 1; }
done

if [[ ! -f "$PRISTINE" ]]; then
  cp -p "$BINK" "$PRISTINE"
fi
cp -p "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" "$ORIGINAL.installing"
mv -f "$ORIGINAL.installing" "$ORIGINAL"
cp -p "$ROOT/build/libMoltenVK.teso4m4.dylib" "$MVK.installing"
mv -f "$MVK.installing" "$MVK"
cp -p "$ROOT/build/libBink2Macx64.dylib" "$BINK.installing"
mv -f "$BINK.installing" "$BINK"

printf '%s\n' "${TESO4M4_MODE:-live-check}" > "$MARKER"
if [[ -f "$PIPELINE_CACHE" && ! -e "$OLD_PIPELINE_CACHE" ]]; then
  mv "$PIPELINE_CACHE" "$OLD_PIPELINE_CACHE"
fi
echo "Installed experimental teso4m4 bridge in ${TESO4M4_MODE:-live-check} mode."
