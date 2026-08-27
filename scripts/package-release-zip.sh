#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
VERSION="${1:-0.1.0-dev}"
NAME="ESO-MoltenVK-Patcher-$VERSION"
STAGE="$ROOT/dist/$NAME"
OUTPUT="$ROOT/dist/$NAME.zip"
INTERNAL="$STAGE/.eso-moltenvk-patcher"
TARGET_NAME="$(<"$ROOT/config/current-target.txt")"
TARGET="$ROOT/config/$TARGET_NAME"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
LEGACY_MVK="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"

"$ROOT/scripts/build.sh"
rm -rf "$STAGE"
mkdir -p "$INTERNAL/bin" "$INTERNAL/payload"
cp "$ROOT/release/bin/eso-moltenvk-patcher" "$INTERNAL/bin/"
cp "$ROOT/release/status.command" "$INTERNAL/"
cp "$ROOT/release/Install.command" "$ROOT/release/Uninstall.command" \
  "$ROOT/release/Status.command" "$ROOT/release/Diagnostics.command" "$STAGE/"
cp "$ROOT/release/README.txt" "$STAGE/"
cp "$ROOT/build/libBink2Macx64.dylib" "$ROOT/build/libMoltenVK.teso4m4.dylib" \
  "$ROOT/build/eso-compat-audit" "$INTERNAL/payload/"
cp "$ROOT/config/usersettings-m4-moltenvk-1.4.2-standard.txt" "$INTERNAL/payload/"
cp "$ROOT/config/settings-profile.env" "$INTERNAL/payload/"
python3 - "$TARGET" "$INTERNAL/payload/target-profile.env" "$VERSION" <<'PY'
import json
import shlex
import sys

profile = json.load(open(sys.argv[1], encoding="utf-8"))
required = ("description", "sha256", "original_bink_sha256", "retagged_original_bink_sha256", "uuid")
missing = [key for key in required if key not in profile]
if missing:
    raise SystemExit(f"Target profile lacks release fields: {', '.join(missing)}")
with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as output:
    output.write(f"RELEASE_VERSION={shlex.quote(sys.argv[3])}\n")
    output.write(f"PROFILE_DESCRIPTION={shlex.quote(profile['description'])}\n")
    output.write(f"EXPECTED_ESO_SHA256={shlex.quote(profile['sha256'])}\n")
    output.write(f"EXPECTED_ORIGINAL_BINK_SHA256={shlex.quote(profile['original_bink_sha256'])}\n")
    output.write(f"EXPECTED_RETAGGED_ORIGINAL_BINK_SHA256={shlex.quote(profile['retagged_original_bink_sha256'])}\n")
    output.write(f"EXPECTED_ESO_UUID={shlex.quote(profile['uuid'])}\n")
PY
print -r -- "EXPECTED_LEGACY_MVK_ARCHIVE_SHA256=$(shasum -a 256 "$LEGACY_MVK" | awk '{print $1}')" \
  >> "$INTERNAL/payload/target-profile.env"
chmod 755 "$INTERNAL/bin/eso-moltenvk-patcher" "$INTERNAL/status.command" "$STAGE"/*.command
chmod 755 "$INTERNAL/payload/eso-compat-audit"
(cd "$ROOT" && ./tools/test_release_installer.sh)
for text_file in "$INTERNAL/bin/eso-moltenvk-patcher" "$INTERNAL/status.command" \
  "$STAGE"/*.command "$STAGE/README.txt" "$INTERNAL/payload/target-profile.env" \
  "$INTERNAL/payload/settings-profile.env" \
  "$INTERNAL/payload/usersettings-m4-moltenvk-1.4.2-standard.txt"; do
  LC_ALL=C grep -q $'\r' "$text_file" && { echo "CRLF is not allowed: $text_file" >&2; exit 1; }
done
(cd "$INTERNAL" && shasum -a 256 \
  ../Install.command ../Uninstall.command ../Status.command \
  ../Diagnostics.command ../README.txt bin/eso-moltenvk-patcher \
  payload/* status.command > SHA256SUMS.txt)
rm -f "$OUTPUT"
(cd "$ROOT/dist" && COPYFILE_DISABLE=1 zip -q -r -X "$OUTPUT" "$NAME")
unzip -tq "$OUTPUT" >/dev/null
metadata_entries="$(unzip -Z1 "$OUTPUT" | grep -E '(^|/)(__MACOSX|\._|\.DS_Store)(/|$)' || true)"
if [[ -n "$metadata_entries" ]]; then
  echo "macOS metadata leaked into the release ZIP." >&2
  echo "$metadata_entries" >&2
  exit 1
fi
"$ROOT/tools/test_release_archive.sh" "$OUTPUT" "$VERSION"
echo "Created $OUTPUT"
