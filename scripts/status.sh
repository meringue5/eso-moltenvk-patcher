#!/bin/zsh
set -euo pipefail

ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
BINK="$GAME_MAC/libBink2Macx64.dylib"
MVK="$GAME_MAC/libMoltenVK.teso4m4.dylib"
MVK_141_BACKUP="$GAME_MAC/libMoltenVK.teso4m4-v1.4.1-backup.dylib"
ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"
EXPECTED_MVK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["analysis"]["replacement_runtime"]["sha256"])' "$MANIFEST")"
ACTUAL_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
MVK_141_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-mvk-1.4.1-backup"

echo "ESO app: $ESO_APP"
echo "ESO SHA-256: $ACTUAL_SHA"
echo "Target manifest: ${MANIFEST:t}"
[[ "$ACTUAL_SHA" == "$EXPECTED_SHA" ]] && echo "ESO target: recognized" || echo "ESO target: UNKNOWN"
if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  echo "Bridge loader: installed"
  BRIDGE_TARGET_SHA="$(strings -a "$BINK" | grep -E '^[0-9a-f]{64}$' | head -1 || true)"
  if [[ -n "$BRIDGE_TARGET_SHA" ]]; then
    echo "Bridge target SHA-256: $BRIDGE_TARGET_SHA"
    [[ "$BRIDGE_TARGET_SHA" == "$ACTUAL_SHA" ]] \
      && echo "Bridge target: current" \
      || echo "Bridge target: STALE (will fail closed)"
  else
    echo "Bridge target: unknown"
  fi
else
  echo "Bridge loader: inactive/original"
fi
[[ -f "$GAME_MAC/.teso4m4-enable" ]] && echo "Enable marker: present" || echo "Enable marker: absent"
if [[ -f "$MVK" ]]; then
  ACTUAL_MVK_SHA="$(shasum -a 256 "$MVK" | awk '{print $1}')"
  echo "Installed MoltenVK SHA-256: $ACTUAL_MVK_SHA"
  [[ "$ACTUAL_MVK_SHA" == "$EXPECTED_MVK_SHA" ]] \
    && echo "Installed MoltenVK: current" \
    || echo "Installed MoltenVK: differs from current target"
else
  echo "Installed MoltenVK: absent"
fi
[[ -f "$MVK_141_BACKUP" ]] \
  && echo "MoltenVK 1.4.1 runtime backup: present" \
  || echo "MoltenVK 1.4.1 runtime backup: absent"
for cache in "$PIPELINE_CACHE" "$OLD_PIPELINE_CACHE" "$MVK_141_PIPELINE_CACHE"; do
  if [[ -f "$cache" ]]; then
    echo "Pipeline cache ${cache:t}: present"
    python3 "$ROOT/tools/pipeline_cache_identity.py" "$cache" || true
  else
    echo "Pipeline cache ${cache:t}: absent"
  fi
done
if [[ -f /tmp/teso4m4.log ]]; then
  echo "--- recent bridge log ---"
  tail -30 /tmp/teso4m4.log
fi
