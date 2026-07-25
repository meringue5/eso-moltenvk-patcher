#!/bin/zsh
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 evidence-directory"
  exit 2
fi

ROOT="${0:A:h:h}"
OUTPUT="$1"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
SETTINGS="$ESO_LIVE/UserSettings.txt"
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
  : > "$OUTPUT/bridge-log-after.txt"
fi
VERDICT_EXIT=0
python3 "$ROOT/tools/check_startup_log.py" \
  "$OUTPUT/bridge-log-after.txt" --after-epoch "$START_EPOCH" \
  > "$OUTPUT/startup-verdict.txt" 2>&1 || VERDICT_EXIT=$?
echo "$VERDICT_EXIT" > "$OUTPUT/startup-verdict-exit-code.txt"
if (( VERDICT_EXIT != 0 )) \
  && grep -Fq -- "- no active instrumented run matched the time gate" \
    "$OUTPUT/startup-verdict.txt"; then
  RETROSPECTIVE_EXIT=0
  python3 "$ROOT/tools/check_startup_log.py" \
    "$OUTPUT/bridge-log-after.txt" \
    > "$OUTPUT/startup-verdict-retrospective.txt" 2>&1 \
    || RETROSPECTIVE_EXIT=$?
  echo "$RETROSPECTIVE_EXIT" \
    > "$OUTPUT/startup-verdict-retrospective-exit-code.txt"
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

START_LOCAL="$(date -r "$START_EPOCH" '+%Y-%m-%d %H:%M:%S')"
/usr/bin/log show --style syslog --start "$START_LOCAL" --predicate 'process == "eso"' \
  > "$OUTPUT/eso-unified.log" 2> "$OUTPUT/eso-unified.stderr" || true
FOCUS_PID="$(
  sed -nE 's/^startup run: .*pid([0-9]+)$/\1/p' \
    "$OUTPUT/startup-verdict.txt"
)"
FOCUS_ANALYSIS_EXIT=0
if [[ "$FOCUS_PID" == <-> ]]; then
  /usr/bin/log show --style compact --start "$START_LOCAL" \
    --predicate \
    "(process == \"runningboardd\" OR process == \"WindowServer\" OR process == \"Dock\" OR process == \"loginwindow\" OR process == \"tccd\") AND (eventMessage CONTAINS[c] \"$FOCUS_PID\" OR eventMessage CONTAINS[c] \"com.zenimaxonline.eso\" OR eventMessage CONTAINS[c] \"zosSteamStarterMac2\")" \
    > "$OUTPUT/window-focus.log" 2> "$OUTPUT/window-focus.stderr" || true
  python3 "$ROOT/tools/analyze_focus_log.py" \
    "$OUTPUT/window-focus.log" --pid "$FOCUS_PID" \
    > "$OUTPUT/focus-verdict.txt" 2>&1 || FOCUS_ANALYSIS_EXIT=$?
else
  echo "No eligible startup PID was available." \
    > "$OUTPUT/focus-verdict.txt"
  FOCUS_ANALYSIS_EXIT=2
fi
echo "$FOCUS_ANALYSIS_EXIT" > "$OUTPUT/focus-verdict-exit-code.txt"

for LOG_NAME in client interface; do
  SOURCE_LOG="$ESO_LIVE/Logs/$LOG_NAME.log"
  if [[ -f "$SOURCE_LOG" ]]; then
    cp -p "$SOURCE_LOG" "$OUTPUT/eso-$LOG_NAME.log"
  fi
done
RESET_ANALYSIS_EXIT=0
if [[ -f "$OUTPUT/eso-client.log" ]]; then
  python3 "$ROOT/tools/analyze_reset_log.py" "$OUTPUT/eso-client.log" \
    > "$OUTPUT/reset-events.txt" 2>&1 || RESET_ANALYSIS_EXIT=$?
fi
echo "$RESET_ANALYSIS_EXIT" > "$OUTPUT/reset-events-exit-code.txt"
LIFECYCLE_ANALYSIS_EXIT=0
python3 "$ROOT/tools/analyze_lifecycle_log.py" \
  "$OUTPUT/bridge-log-after.txt" --after-epoch "$START_EPOCH" \
  > "$OUTPUT/lifecycle-events.txt" 2>&1 \
  || LIFECYCLE_ANALYSIS_EXIT=$?
echo "$LIFECYCLE_ANALYSIS_EXIT" \
  > "$OUTPUT/lifecycle-events-exit-code.txt"

PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
{
  if [[ -f "$PIPELINE_CACHE" ]]; then
    stat -f 'active bytes=%z modified_epoch=%m' "$PIPELINE_CACHE"
    shasum -a 256 "$PIPELINE_CACHE"
  else
    echo "active absent"
  fi
  if [[ -f "$OLD_PIPELINE_CACHE" ]]; then
    stat -f 'old-backup bytes=%z modified_epoch=%m' "$OLD_PIPELINE_CACHE"
    shasum -a 256 "$OLD_PIPELINE_CACHE"
  else
    echo "old-backup absent"
  fi
} > "$OUTPUT/pipeline-cache-fingerprints.txt"

"$ROOT/scripts/status.sh" > "$OUTPUT/status-after.txt"
UPDATE_CHECK_EXIT=0
"$ROOT/scripts/check-update.sh" > "$OUTPUT/update-check-after.txt" 2>&1 \
  || UPDATE_CHECK_EXIT=$?
echo "$UPDATE_CHECK_EXIT" > "$OUTPUT/update-check-after-exit-code.txt"
QUICK_CHECK_EXIT=0
"$ROOT/scripts/quick-update-check.sh" \
  > "$OUTPUT/quick-update-check-after.txt" 2>&1 || QUICK_CHECK_EXIT=$?
echo "$QUICK_CHECK_EXIT" > "$OUTPUT/quick-update-check-after-exit-code.txt"
LAUNCHER_CHECK_EXIT=0
"$ROOT/scripts/check-launcher-state.sh" \
  --max-age-seconds "${TESO4M4_LAUNCHER_MAX_AGE_SECONDS:-3600}" \
  > "$OUTPUT/launcher-state-after.txt" 2>&1 || LAUNCHER_CHECK_EXIT=$?
echo "$LAUNCHER_CHECK_EXIT" > "$OUTPUT/launcher-state-after-exit-code.txt"

{
  if [[ -f "$SETTINGS" ]]; then
    cp -p "$SETTINGS" "$OUTPUT/UserSettings-after.txt"
    stat -f 'bytes=%z modified_epoch=%m' "$SETTINGS"
    shasum -a 256 "$SETTINGS"
    grep -E '^SET (AMBIENT_OCCLUSION_TYPE|SkipPregameVideos) "[01]"$' \
      "$SETTINGS" || true
  else
    echo "settings absent"
  fi
} > "$OUTPUT/settings-after.txt"
SETTINGS_DIFF_EXIT=0
if [[ -f "$OUTPUT/UserSettings-before.txt" \
  && -f "$OUTPUT/UserSettings-after.txt" ]]; then
  python3 "$ROOT/tools/settings_diff.py" \
    "$OUTPUT/UserSettings-before.txt" "$OUTPUT/UserSettings-after.txt" \
    > "$OUTPUT/settings-diff.txt" 2>&1 || SETTINGS_DIFF_EXIT=$?
else
  echo "Before/after settings pair is unavailable." \
    > "$OUTPUT/settings-diff.txt"
  SETTINGS_DIFF_EXIT=2
fi
echo "$SETTINGS_DIFF_EXIT" > "$OUTPUT/settings-diff-exit-code.txt"

(
  cd "$OUTPUT"
  : > SHA256SUMS
  for FILE in *(N.); do
    [[ "$FILE" == SHA256SUMS ]] && continue
    shasum -a 256 "$FILE" >> SHA256SUMS
  done
)

echo "Evidence collected in: $OUTPUT"
echo "Crash reports copied: $REPORT_COUNT"
[[ -f "$OUTPUT/startup-verdict.txt" ]] && cat "$OUTPUT/startup-verdict.txt"
echo "Raw evidence may contain local paths or identifiers; do not commit it."
