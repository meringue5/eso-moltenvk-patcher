#!/bin/zsh
set -euo pipefail

if [[ $# -ne 1 || "$1" != (0|1) ]]; then
  echo "usage: $0 0|1"
  exit 2
fi

NEW_VALUE="$1"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
SETTINGS="$ESO_LIVE/UserSettings.txt"

if pgrep -x eso >/dev/null 2>&1; then
  echo "ESO is running; refusing to modify UserSettings.txt."
  exit 1
fi
[[ -f "$SETTINGS" ]] || {
  echo "Missing settings file: $SETTINGS"
  exit 1
}

LINE_COUNT="$(grep -Ec '^SET AMBIENT_OCCLUSION_TYPE "[01]"$' "$SETTINGS" || true)"
[[ "$LINE_COUNT" == 1 ]] || {
  echo "Expected exactly one supported AMBIENT_OCCLUSION_TYPE line; found $LINE_COUNT."
  exit 1
}
OLD_VALUE="$(sed -n 's/^SET AMBIENT_OCCLUSION_TYPE "\([01]\)"$/\1/p' "$SETTINGS")"

if [[ "$OLD_VALUE" == "$NEW_VALUE" ]]; then
  echo "AMBIENT_OCCLUSION_TYPE is already $NEW_VALUE; no file was changed."
  exit 0
fi

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
BACKUP="$SETTINGS.teso4m4-before-ao-${OLD_VALUE}-to-${NEW_VALUE}-${STAMP}"
[[ ! -e "$BACKUP" ]] || {
  echo "Backup path already exists: $BACKUP"
  exit 1
}

TEMP="$(mktemp "$ESO_LIVE/.UserSettings.txt.teso4m4.XXXXXX")"
cleanup() {
  [[ -n "${TEMP:-}" && -e "$TEMP" ]] && rm -f -- "$TEMP"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

ORIGINAL_HASH="$(shasum -a 256 "$SETTINGS" | awk '{print $1}')"
cp -p "$SETTINGS" "$BACKUP"
BACKUP_HASH="$(shasum -a 256 "$BACKUP" | awk '{print $1}')"
[[ "$BACKUP_HASH" == "$ORIGINAL_HASH" ]] || {
  echo "Backup verification failed; original settings remain unchanged."
  exit 1
}

cp -p "$SETTINGS" "$TEMP"
sed -i '' -e "s/^SET AMBIENT_OCCLUSION_TYPE \"${OLD_VALUE}\"\$/SET AMBIENT_OCCLUSION_TYPE \"${NEW_VALUE}\"/" "$TEMP"
NEW_LINE_COUNT="$(grep -Ec "^SET AMBIENT_OCCLUSION_TYPE \"${NEW_VALUE}\"\$" "$TEMP" || true)"
[[ "$NEW_LINE_COUNT" == 1 ]] || {
  echo "Replacement verification failed; original settings remain unchanged."
  exit 1
}

mv -f "$TEMP" "$SETTINGS"
TEMP=""
NEW_HASH="$(shasum -a 256 "$SETTINGS" | awk '{print $1}')"

echo "AMBIENT_OCCLUSION_TYPE: $OLD_VALUE -> $NEW_VALUE"
echo "Backup: $BACKUP"
echo "Backup SHA-256: $BACKUP_HASH"
echo "Updated SHA-256: $NEW_HASH"
