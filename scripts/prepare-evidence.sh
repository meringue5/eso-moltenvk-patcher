#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUTPUT="${1:-$ROOT/artifacts/experiment-0006-$STAMP}"
OUTPUT="${OUTPUT:A}"
PROXY="$ROOT/build/libBink2Macx64.dylib"
MOLTENVK="$ROOT/build/libMoltenVK.teso4m4.dylib"
ESO_LIVE="${ESO_LIVE:-$HOME/Documents/Elder Scrolls Online/live}"
SETTINGS="$ESO_LIVE/UserSettings.txt"
PIPELINE_CACHE="$ESO_LIVE/PipelineCache.esopc"
OLD_PIPELINE_CACHE="${PIPELINE_CACHE}.teso4m4-old-backup"
UPDATE_CHECK_EXIT=0
UPDATE_CHECK="$("$ROOT/scripts/check-update.sh" 2>&1)" || UPDATE_CHECK_EXIT=$?
QUICK_CHECK_EXIT=0
QUICK_CHECK="$("$ROOT/scripts/quick-update-check.sh" 2>&1)" \
  || QUICK_CHECK_EXIT=$?

if [[ -e "$OUTPUT" ]]; then
  echo "Evidence directory already exists: $OUTPUT"
  exit 1
fi
[[ -f "$PROXY" && -f "$MOLTENVK" ]] || {
  echo "Missing built bridge artifacts; run scripts/build.sh first."
  exit 1
}
if (( UPDATE_CHECK_EXIT != 0 )); then
  echo "$UPDATE_CHECK"
  echo "ESO target changed; refusing to prepare stale experiment evidence."
  exit "$UPDATE_CHECK_EXIT"
fi
if (( QUICK_CHECK_EXIT != 0 )); then
  echo "$QUICK_CHECK"
  echo "Quick update gate stopped; refusing to prepare ambiguous evidence."
  exit "$QUICK_CHECK_EXIT"
fi
[[ -z "$(git -C "$ROOT" status --porcelain --untracked-files=normal)" ]] || {
  echo "Source worktree is not clean; refusing to prepare ambiguous evidence."
  exit 1
}
mkdir -p "$OUTPUT"
echo "$UPDATE_CHECK" > "$OUTPUT/update-check-before.txt"
echo "$QUICK_CHECK" > "$OUTPUT/quick-update-check-before.txt"
"$ROOT/scripts/check-launcher-state.sh" \
  --max-age-seconds "${TESO4M4_LAUNCHER_MAX_AGE_SECONDS:-3600}" \
  > "$OUTPUT/launcher-state-before.txt"
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
{
  if [[ -f "$SETTINGS" ]]; then
    cp -p "$SETTINGS" "$OUTPUT/UserSettings-before.txt"
    stat -f 'bytes=%z modified_epoch=%m' "$SETTINGS"
    shasum -a 256 "$SETTINGS"
    grep -E '^SET (AMBIENT_OCCLUSION_TYPE|SkipPregameVideos) "[01]"$' \
      "$SETTINGS" || true
  else
    echo "settings absent"
  fi
} > "$OUTPUT/settings-baseline.txt"
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
} > "$OUTPUT/pipeline-cache-fingerprints-before.txt"

echo "Evidence collection prepared: $OUTPUT"
echo "After the user-controlled startup test, run:"
echo "  $ROOT/scripts/collect-evidence.sh ${(q)OUTPUT}"
