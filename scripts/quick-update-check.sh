#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
BINK="$GAME_MAC/libBink2Macx64.dylib"
MARKER="$GAME_MAC/.teso4m4-enable"
MAX_AGE_SECONDS="${TESO4M4_LAUNCHER_MAX_AGE_SECONDS:-3600}"

eso_exit=0
launcher_exit=0
eso_result="$("$ROOT/scripts/check-update.sh" 2>&1)" || eso_exit=$?
launcher_result="$("$ROOT/scripts/check-launcher-state.sh" \
  --max-age-seconds "$MAX_AGE_SECONDS" 2>&1)" \
  || launcher_exit=$?

eso_state="$(print -r -- "$eso_result" \
  | sed -n 's/^ESO update status: //p' | head -1)"
eso_sha="$(print -r -- "$eso_result" \
  | sed -n 's/^ESO SHA-256: //p' | head -1)"
launcher_state="$(print -r -- "$launcher_result" \
  | sed -n 's/^Launcher Live_Prod state: //p' | head -1)"
launcher_time="$(print -r -- "$launcher_result" \
  | sed -n 's/^State check time: //p' | head -1)"

bridge_state="inactive"
bridge_sha=""
if [[ -f "$BINK" ]] && otool -L "$BINK" | grep -q 'teso4m4-original'; then
  bridge_sha="$(strings -a "$BINK" \
    | grep -E '^[0-9a-f]{64}$' | head -1 || true)"
  if [[ -n "$bridge_sha" && "$bridge_sha" == "$eso_sha" ]]; then
    bridge_state="current"
  else
    bridge_state="stale-or-unknown"
  fi
fi

marker_state="absent"
[[ -f "$MARKER" ]] && marker_state="present"

ready=true
(( eso_exit == 0 )) || ready=false
(( launcher_exit == 0 )) || ready=false
[[ "$eso_state" == "CURRENT" ]] || ready=false
[[ "$launcher_state" == "CURRENT_REMOTE" ]] || ready=false
[[ "$bridge_state" == "current" ]] || ready=false
[[ "$marker_state" == "present" ]] || ready=false

if [[ "$ready" == true ]]; then
  echo "TESO4M4 quick gate: READY"
  echo "ESO target/content: $eso_state"
  echo "Launcher content: $launcher_state ($launcher_time)"
  echo "Bridge checkpoint: current and enabled"
  exit 0
fi

echo "TESO4M4 quick gate: STOP"
echo "ESO target/content: ${eso_state:-INDETERMINATE}"
echo "Launcher content: ${launcher_state:-INDETERMINATE}${launcher_time:+ ($launcher_time)}"
echo "Bridge checkpoint: $bridge_state; marker $marker_state"
if (( eso_exit != 0 )); then
  echo "--- ESO target detail ---"
  print -r -- "$eso_result"
fi
if (( launcher_exit != 0 )); then
  echo "--- launcher detail ---"
  print -r -- "$launcher_result"
fi
exit 3
