#!/bin/zsh
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 evidence-directory"
  exit 2
fi

ROOT="${0:A:h:h}"
OUTPUT="$1"
START_EPOCH_FILE="$OUTPUT/started-at-epoch.txt"
START_UTC_FILE="$OUTPUT/started-at-utc.txt"
[[ -d "$OUTPUT" ]] || { echo "Missing evidence directory: $OUTPUT"; exit 1; }
[[ -f "$START_EPOCH_FILE" && -f "$START_UTC_FILE" ]] || {
  echo "Not a prepared evidence directory: $OUTPUT"
  exit 1
}
START_EPOCH="$(<"$START_EPOCH_FILE")"
[[ "$START_EPOCH" == <-> ]] || { echo "Invalid start epoch"; exit 1; }

if [[ -f /tmp/teso4m4.log ]]; then
  cp -p /tmp/teso4m4.log "$OUTPUT/bridge-log-after.txt"
else
  echo "No /tmp/teso4m4.log was present." > "$OUTPUT/bridge-log-missing.txt"
fi

REPORT_COUNT=0
for DIRECTORY in "$HOME/Library/Logs/DiagnosticReports" "/Library/Logs/DiagnosticReports"; do
  [[ -d "$DIRECTORY" ]] || continue
  PREFIX="user"
  [[ "$DIRECTORY" == /Library/* ]] && PREFIX="system"
  for REPORT in "$DIRECTORY"/eso*.ips(N); do
    MODIFIED="$(stat -f %m "$REPORT")"
    if (( MODIFIED >= START_EPOCH )); then
      cp -p "$REPORT" "$OUTPUT/$PREFIX-${REPORT:t}"
      (( REPORT_COUNT += 1 ))
    fi
  done
done
echo "$REPORT_COUNT" > "$OUTPUT/crash-report-count.txt"

START_UTC="$(<"$START_UTC_FILE")"
log show --style syslog --start "$START_UTC" --predicate 'process == "eso"' \
  > "$OUTPUT/eso-unified.log" 2> "$OUTPUT/eso-unified.stderr" || true
"$ROOT/scripts/status.sh" > "$OUTPUT/status-after.txt"

echo "Evidence collected in: $OUTPUT"
echo "Crash reports copied: $REPORT_COUNT"
echo "Raw evidence may contain local paths or identifiers; do not commit it."
