#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUTPUT="${1:-$ROOT/artifacts/experiment-0006-$STAMP}"
OUTPUT="${OUTPUT:A}"
PROXY="$ROOT/build/libBink2Macx64.dylib"
MOLTENVK="$ROOT/build/libMoltenVK.teso4m4.dylib"

if [[ -e "$OUTPUT" ]]; then
  echo "Evidence directory already exists: $OUTPUT"
  exit 1
fi
[[ -f "$PROXY" && -f "$MOLTENVK" ]] || {
  echo "Missing built bridge artifacts; run scripts/build.sh first."
  exit 1
}
[[ -z "$(git -C "$ROOT" status --porcelain --untracked-files=normal)" ]] || {
  echo "Source worktree is not clean; refusing to prepare ambiguous evidence."
  exit 1
}
mkdir -p "$OUTPUT"
date -u +%Y-%m-%dT%H:%M:%SZ > "$OUTPUT/started-at-utc.txt"
date +%s > "$OUTPUT/started-at-epoch.txt"
git -C "$ROOT" rev-parse HEAD > "$OUTPUT/source-commit.txt"
(
  cd "$ROOT/build"
  shasum -a 256 libBink2Macx64.dylib libMoltenVK.teso4m4.dylib \
    > "$OUTPUT/built-artifacts-sha256.txt"
)
"$ROOT/scripts/status.sh" > "$OUTPUT/status-before.txt"
if [[ -f /tmp/teso4m4.log ]]; then
  cp -p /tmp/teso4m4.log "$OUTPUT/bridge-log-before.txt"
fi

echo "Evidence collection prepared: $OUTPUT"
echo "After the user-controlled startup test, run:"
echo "  $ROOT/scripts/collect-evidence.sh ${(q)OUTPUT}"
