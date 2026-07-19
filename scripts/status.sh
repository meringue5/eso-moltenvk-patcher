#!/bin/zsh
set -euo pipefail

ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
BINK="$GAME_MAC/libBink2Macx64.dylib"

echo "ESO app: $ESO_APP"
echo "ESO SHA-256: $(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  echo "Bridge loader: installed"
else
  echo "Bridge loader: inactive/original"
fi
[[ -f "$GAME_MAC/.teso4m4-enable" ]] && echo "Enable marker: present" || echo "Enable marker: absent"
if [[ -f /tmp/teso4m4.log ]]; then
  echo "--- recent bridge log ---"
  tail -30 /tmp/teso4m4.log
fi

