#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUTPUT="${1:-$ROOT/artifacts/experiment-0002-$STAMP}"

if [[ -e "$OUTPUT" ]]; then
  echo "Evidence directory already exists: $OUTPUT"
  exit 1
fi
mkdir -p "$OUTPUT"
date -u +%Y-%m-%dT%H:%M:%SZ > "$OUTPUT/started-at-utc.txt"
date +%s > "$OUTPUT/started-at-epoch.txt"
"$ROOT/scripts/status.sh" > "$OUTPUT/status-before.txt"
if [[ -f /tmp/teso4m4.log ]]; then
  cp -p /tmp/teso4m4.log "$OUTPUT/bridge-log-before.txt"
fi

echo "Evidence collection prepared: $OUTPUT"
echo "After the user-controlled startup test, run:"
echo "  $ROOT/scripts/collect-evidence.sh ${(q)OUTPUT}"
