#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
VERSION="${1:-0.1.0-dev}"
NAME="ESO-MoltenVK-Patcher-$VERSION"
STAGE="$ROOT/dist/$NAME"
OUTPUT="$ROOT/dist/$NAME.zip"
TARGET_NAME="$(<"$ROOT/config/current-target.txt")"
TARGET="$ROOT/config/$TARGET_NAME"

"$ROOT/scripts/build.sh"
rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/payload"
cp "$ROOT/release/bin/eso-moltenvk-patcher" "$STAGE/bin/"
cp "$ROOT/release/install.command" "$ROOT/release/status.command" \
  "$ROOT/release/remove.command" "$STAGE/"
cp "$ROOT/release/README.txt" "$STAGE/"
cp "$ROOT/build/libBink2Macx64.dylib" "$ROOT/build/libMoltenVK.teso4m4.dylib" \
  "$STAGE/payload/"
cp "$ROOT/config/usersettings-m4-moltenvk-1.4.2-standard.txt" "$STAGE/payload/"
python3 - "$TARGET" "$STAGE/payload/target-profile.env" "$VERSION" <<'PY'
import json
import shlex
import sys

profile = json.load(open(sys.argv[1], encoding="utf-8"))
required = ("description", "sha256", "original_bink_sha256", "uuid")
missing = [key for key in required if key not in profile]
if missing:
    raise SystemExit(f"Target profile lacks release fields: {', '.join(missing)}")
with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as output:
    output.write(f"RELEASE_VERSION={shlex.quote(sys.argv[3])}\n")
    output.write(f"PROFILE_DESCRIPTION={shlex.quote(profile['description'])}\n")
    output.write(f"EXPECTED_ESO_SHA256={shlex.quote(profile['sha256'])}\n")
    output.write(f"EXPECTED_ORIGINAL_BINK_SHA256={shlex.quote(profile['original_bink_sha256'])}\n")
    output.write(f"EXPECTED_ESO_UUID={shlex.quote(profile['uuid'])}\n")
PY
chmod 755 "$STAGE/bin/eso-moltenvk-patcher" "$STAGE"/*.command
(cd "$ROOT" && ./tools/test_release_installer.sh)
for text_file in "$STAGE/bin/eso-moltenvk-patcher" "$STAGE"/*.command "$STAGE/README.txt" "$STAGE/payload/target-profile.env"; do
  LC_ALL=C grep -q $'\r' "$text_file" && { echo "CRLF is not allowed: $text_file" >&2; exit 1; }
done
(cd "$STAGE" && shasum -a 256 bin/eso-moltenvk-patcher payload/* *.command README.txt > SHA256SUMS.txt)
rm -f "$OUTPUT"
(cd "$ROOT/dist" && COPYFILE_DISABLE=1 zip -q -r -X "$OUTPUT" "$NAME")
unzip -tq "$OUTPUT" >/dev/null
metadata_entries="$(unzip -Z1 "$OUTPUT" | grep -E '(^|/)(__MACOSX|\._|\.DS_Store)(/|$)' || true)"
if [[ -n "$metadata_entries" ]]; then
  echo "macOS metadata leaked into the release ZIP." >&2
  echo "$metadata_entries" >&2
  exit 1
fi
echo "Created $OUTPUT"
