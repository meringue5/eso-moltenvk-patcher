#!/bin/zsh
set -euo pipefail

if [[ "${TESO4M4_EXPERIMENTAL:-}" != "I_ACCEPT_CRASH_RISK" ]]; then
  echo "This source-maintenance command changes the ESO application bundle."
  echo "Players should use the versioned public release installer instead."
  echo "Read docs/STATUS.md and set TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK"
  echo "only for verified production maintenance on an exact supported target."
  exit 1
fi

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
MODE="${TESO4M4_MODE:-startup-pipeline-timing-control}"
[[ "$MODE" == "performance-aggressive" \
  || "$MODE" == "startup-compositor-neutralize" \
  || "$MODE" == "startup-pipeline-timing-control" \
  || "$MODE" == "startup-inactive-pacing-bypass" \
  || "$MODE" == "startup-compositor-audit-pacing-bypass" \
  || "$MODE" == "startup-compositor-neutralize-pacing-bypass" \
  || "$MODE" == "startup-compositor-neutralize-pacing-release" ]] || {
  echo "Unsupported production maintenance mode: $MODE"
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
EXPECTED_ORIGINAL_BINK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["original_bink_sha256"])' "$MANIFEST")"
EXPECTED_MVK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["analysis"]["replacement_runtime"]["sha256"])' "$MANIFEST")"

teso4m4_require_bundle_idle "$ESO_APP"
if [[ -n "$PRESERVE_CACHE_STATE" && "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Unsupported TESO4M4_PRESERVE_CACHE_STATE value."
  exit 1
fi
actual_sha="$(shasum -a 256 "$ESO" | awk '{print $1}')"
[[ "$actual_sha" == "$EXPECTED_SHA" ]] || { echo "Unknown ESO build; refusing."; exit 1; }
for file in libBink2Macx64.dylib libBink2Macx64.teso4m4-original.dylib libMoltenVK.teso4m4.dylib; do
  [[ -f "$ROOT/build/$file" ]] || { echo "Run scripts/build.sh first."; exit 1; }
done
build_mvk_sha="$(shasum -a 256 "$ROOT/build/libMoltenVK.teso4m4.dylib" | awk '{print $1}')"
[[ "$build_mvk_sha" == "$EXPECTED_MVK_SHA" ]] || {
  echo "Built replacement MoltenVK does not match the selected target profile."
  echo "Expected: $EXPECTED_MVK_SHA"
  echo "Actual:   $build_mvk_sha"
  exit 1
}

if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  echo "Active Bink is already a bridge; restore before installing."
  exit 1
fi
[[ ! -e "$MARKER" ]] || {
  echo "Enable marker already exists; restore and re-check status first."
  exit 1
}
[[ "$(shasum -a 256 "$BINK" | awk '{print $1}')" == "$EXPECTED_ORIGINAL_BINK_SHA" ]] || {
  echo "Active Bink does not match the original-loader generation selected by this target; refusing."
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
[[ "$(shasum -a 256 "$PRISTINE" | awk '{print $1}')" == "$EXPECTED_ORIGINAL_BINK_SHA" ]] || {
  echo "Pristine backup belongs to a different original-loader generation; refusing."
  exit 1
}
cmp -s "$BINK" "$PRISTINE" || {
  echo "Active Bink does not match the pristine restore source; refusing."
  exit 1
}
if [[ -e "$ORIGINAL" ]]; then
  cmp -s "$ORIGINAL" "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" || {
    echo "Existing renamed original differs from the current source build."
    exit 1
  }
fi
cp -p "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" "$ORIGINAL.installing"
cp -p "$ROOT/build/libMoltenVK.teso4m4.dylib" "$MVK.installing"
cp -p "$ROOT/build/libBink2Macx64.dylib" "$BINK.installing"
cmp -s "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" "$ORIGINAL.installing"
cmp -s "$ROOT/build/libMoltenVK.teso4m4.dylib" "$MVK.installing"
cmp -s "$ROOT/build/libBink2Macx64.dylib" "$BINK.installing"

mv -f "$ORIGINAL.installing" "$ORIGINAL"
mv -f "$MVK.installing" "$MVK"
mv -f "$BINK.installing" "$BINK"

printf '%s\n' "$MODE" > "$MARKER.installing"
mv "$MARKER.installing" "$MARKER"
if [[ "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" \
  && -f "$PIPELINE_CACHE" && ! -e "$OLD_PIPELINE_CACHE" ]]; then
  mv "$PIPELINE_CACHE" "$OLD_PIPELINE_CACHE"
fi
if [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Installed production bridge in $MODE mode; preserved both pipeline caches in place."
else
  echo "Installed production bridge in $MODE mode."
fi
