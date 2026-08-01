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
  || "$MODE" == "performance-safe" \
  || "$MODE" == "performance-aggressive" \
  || "$MODE" == "startup-color-audit" \
  || "$MODE" == "startup-fx-neutralize" \
  || "$MODE" == "startup-present-pixel-audit" \
  || "$MODE" == "startup-draw-audit" \
  || "$MODE" == "startup-input-audit" \
  || "$MODE" == "startup-compositor-audit" \
  || "$MODE" == "startup-compositor-neutralize" ]] || {
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
MVK_141_BACKUP="$GAME_MAC/libMoltenVK.teso4m4-v1.4.1-backup.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
MVK_141_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-mvk-1.4.1-backup"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
PRESERVE_CACHE_STATE="${TESO4M4_PRESERVE_CACHE_STATE:-}"
CACHE_TRANSITION="${TESO4M4_CACHE_TRANSITION:-}"
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"
EXPECTED_MVK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["analysis"]["replacement_runtime"]["sha256"])' "$MANIFEST")"
MVK_141_SHA="d3ee87b2d98c0b7d5db7bcd1e51b010fe998f755f26c09a83768275499b7a398"
MVK_141_CACHE_UUID="db445ff21a0502090000000100000000"

teso4m4_require_bundle_idle "$ESO_APP"
if [[ -n "$PRESERVE_CACHE_STATE" && "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Unsupported TESO4M4_PRESERVE_CACHE_STATE value."
  exit 1
fi
if [[ -n "$CACHE_TRANSITION" && "$CACHE_TRANSITION" != "mvk-1.4.1-to-1.4.2" ]]; then
  echo "Unsupported TESO4M4_CACHE_TRANSITION value."
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
if [[ "$CACHE_TRANSITION" == "mvk-1.4.1-to-1.4.2" ]]; then
  [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]] || {
    echo "The 1.4.1-to-1.4.2 cache transition requires explicit cache preservation."
    exit 1
  }
  [[ -f "$MVK" ]] || {
    echo "Installed MoltenVK 1.4.1 runtime is missing; refusing transition."
    exit 1
  }
  installed_mvk_sha="$(shasum -a 256 "$MVK" | awk '{print $1}')"
  [[ "$installed_mvk_sha" == "$MVK_141_SHA" ]] || {
    echo "Installed MoltenVK is not the exact preserved 1.4.1 runtime."
    echo "Expected: $MVK_141_SHA"
    echo "Actual:   $installed_mvk_sha"
    exit 1
  }
  [[ ! -e "$MVK_141_BACKUP" && ! -e "$MVK_141_PIPELINE_CACHE" ]] || {
    echo "A versioned MoltenVK 1.4.1 runtime or cache backup already exists."
    exit 1
  }
  python3 "$ROOT/tools/pipeline_cache_identity.py" \
    "$PIPELINE_CACHE" --expect-uuid "$MVK_141_CACHE_UUID"
fi
if [[ ! -f "$PRISTINE" ]]; then
  cp -p "$BINK" "$PRISTINE"
fi
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

if [[ "$CACHE_TRANSITION" == "mvk-1.4.1-to-1.4.2" ]]; then
  cp -p "$MVK" "$MVK_141_BACKUP.installing"
  cmp -s "$MVK" "$MVK_141_BACKUP.installing"
  mv "$MVK_141_BACKUP.installing" "$MVK_141_BACKUP"
  mv "$PIPELINE_CACHE" "$MVK_141_PIPELINE_CACHE"
fi

mv -f "$ORIGINAL.installing" "$ORIGINAL"
mv -f "$MVK.installing" "$MVK"
mv -f "$BINK.installing" "$BINK"

printf '%s\n' "$MODE" > "$MARKER.installing"
mv "$MARKER.installing" "$MARKER"
if [[ "$PRESERVE_CACHE_STATE" != "I_ACCEPT_EXISTING_CACHE_STATE" \
  && -f "$PIPELINE_CACHE" && ! -e "$OLD_PIPELINE_CACHE" ]]; then
  mv "$PIPELINE_CACHE" "$OLD_PIPELINE_CACHE"
fi
if [[ "$CACHE_TRANSITION" == "mvk-1.4.1-to-1.4.2" ]]; then
  echo "Installed experimental teso4m4 bridge in $MODE mode."
  echo "Preserved MoltenVK 1.4.1 and its active cache under versioned backup names."
  echo "MoltenVK 1.4.2 will create a separate cold pipeline cache."
elif [[ "$PRESERVE_CACHE_STATE" == "I_ACCEPT_EXISTING_CACHE_STATE" ]]; then
  echo "Installed experimental teso4m4 bridge in $MODE mode; preserved both pipeline caches in place."
else
  echo "Installed experimental teso4m4 bridge in $MODE mode."
fi
