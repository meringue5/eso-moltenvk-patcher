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
cp "$ROOT/release/install.command" "$ROOT/release/check.command" \
  "$ROOT/release/remove.command" "$STAGE/"
cp "$ROOT/build/libBink2Macx64.dylib" "$ROOT/build/libMoltenVK.teso4m4.dylib" \
  "$STAGE/payload/"
python3 - "$TARGET" "$STAGE/payload/target-profile.env" <<'PY'
import json
import shlex
import sys

profile = json.load(open(sys.argv[1], encoding="utf-8"))
required = ("description", "sha256", "original_bink_sha256", "uuid")
missing = [key for key in required if key not in profile]
if missing:
    raise SystemExit(f"Target profile lacks release fields: {', '.join(missing)}")
with open(sys.argv[2], "w", encoding="utf-8") as output:
    output.write(f"PROFILE_DESCRIPTION={shlex.quote(profile['description'])}\n")
    output.write(f"EXPECTED_ESO_SHA256={shlex.quote(profile['sha256'])}\n")
    output.write(f"EXPECTED_ORIGINAL_BINK_SHA256={shlex.quote(profile['original_bink_sha256'])}\n")
    output.write(f"EXPECTED_ESO_UUID={shlex.quote(profile['uuid'])}\n")
PY
cat > "$STAGE/README.txt" <<'EOF'
ESO MoltenVK Patcher

1. Double-click check.command first, or run it in Terminal.
2. Double-click install.command to install the verified patch.
3. Double-click remove.command to restore the original Bink library.

If ESO is installed elsewhere, run one of the commands in Terminal with:
  --eso-app '/path/to/eso.app'

The scripts never launch ESO, Steam, or the ZeniMax launcher. They only modify
an exact recognized client after its bundle is idle. Quit ESO and the ZeniMax
launcher before Install, Remove, or Repair. Idle Steam alone is allowed when
it is not updating ESO.

For source, release notes, supported build, and troubleshooting, see:
https://github.com/lvcwoo/teso4m4
EOF
chmod 755 "$STAGE/bin/eso-moltenvk-patcher" "$STAGE"/*.command
(cd "$STAGE" && shasum -a 256 bin/eso-moltenvk-patcher payload/* *.command README.txt > SHA256SUMS.txt)
rm -f "$OUTPUT"
COPYFILE_DISABLE=true ditto -c -k --keepParent "$STAGE" "$OUTPUT"
echo "Created $OUTPUT"
