#!/bin/zsh
set -euo pipefail

ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
BINK="$GAME_MAC/libBink2Macx64.dylib"
ROOT="${0:A:h:h}"
MANIFEST="${TESO4M4_TARGET_MANIFEST:-$ROOT/config/targets-eso-2026-07-20.json}"
EXPECTED_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "$MANIFEST")"
ACTUAL_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"

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
if [[ -f /tmp/teso4m4.log ]]; then
  echo "--- recent bridge log ---"
  tail -30 /tmp/teso4m4.log
fi
