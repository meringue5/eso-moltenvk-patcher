#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
PRISTINE="$GAME_MAC/libBink2Macx64.teso4m4-pristine.dylib"
LEGACY_MVK="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"
MVK_ROOT="${MVK_ROOT:-$ROOT/vendor/MoltenVK-1.4.2-official}"
MVK_INCLUDE_ROOT="${MVK_INCLUDE_ROOT:-$MVK_ROOT}"
MVK="$MVK_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
BUILD="$ROOT/build"
EXPECTED_MVK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["analysis"]["replacement_runtime"]["sha256"])' "$MANIFEST")"

for file in "$ESO" "$BINK" "$LEGACY_MVK" "$MVK" "$MANIFEST"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
ACTUAL_MVK_SHA="$(shasum -a 256 "$MVK" | awk '{print $1}')"
[[ "$ACTUAL_MVK_SHA" == "$EXPECTED_MVK_SHA" ]] || {
  echo "Replacement MoltenVK does not match the selected target profile."
  echo "Expected: $EXPECTED_MVK_SHA"
  echo "Actual:   $ACTUAL_MVK_SHA"
  exit 1
}
EXPECTED_ORIGINAL_BINK_SHA="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["original_bink_sha256"])' "$MANIFEST")"
bink_sha() { shasum -a 256 "$1" | awk '{print $1}'; }
SOURCE_BINK="$BINK"
if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  # The release installer keeps the current generation's verified original in
  # its recovery record; read it (never modify it) when the legacy pristine
  # copy belongs to an older Bink generation.
  RECOVERY_BINK="${ESO_MOLTENVK_PATCHER_STATE_ROOT:-$HOME/Library/Application Support/ESO MoltenVK Patcher/Installations}/$(print -rn -- "$ESO_APP" | shasum -a 256 | awk '{print $1}')/original-libBink2Macx64.dylib"
  if [[ -f "$PRISTINE" && "$(bink_sha "$PRISTINE")" == "$EXPECTED_ORIGINAL_BINK_SHA" ]]; then
    SOURCE_BINK="$PRISTINE"
  elif [[ -f "$RECOVERY_BINK" && "$(bink_sha "$RECOVERY_BINK")" == "$EXPECTED_ORIGINAL_BINK_SHA" ]]; then
    SOURCE_BINK="$RECOVERY_BINK"
  elif [[ -f "$PRISTINE" ]]; then
    SOURCE_BINK="$PRISTINE"
  else
    echo "Active Bink is a bridge and the pristine build source is missing."
    exit 1
  fi
fi
# A launcher update can ship a new original Bink generation while an older
# pristine backup is preserved; build only from the selected target's original.
[[ "$(bink_sha "$SOURCE_BINK")" == "$EXPECTED_ORIGINAL_BINK_SHA" ]] || {
  echo "Bink build source does not match the selected target's original Bink."
  exit 1
}

mkdir -p "$BUILD"
python3 "$ROOT/tools/generate_targets.py" "$ESO" "$MANIFEST" "$BUILD/generated_targets.h"
python3 "$ROOT/tools/generate_compat_audit_profile.py" \
  --exe "$ESO" --archive "$LEGACY_MVK" --manifest "$MANIFEST" \
  --output "$BUILD/generated_compat_audit.h"

cp -p "$SOURCE_BINK" "$BUILD/libBink2Macx64.teso4m4-original.dylib"
install_name_tool -id @loader_path/libBink2Macx64.teso4m4-original.dylib \
  "$BUILD/libBink2Macx64.teso4m4-original.dylib"
cp -p "$MVK" "$BUILD/libMoltenVK.teso4m4.dylib"

xcrun clang -fobjc-arc -dynamiclib -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -O2 -I"$BUILD" -I"$ROOT/src" \
  -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/src/mvk_shim.c" "$ROOT/src/mvk_log_file.c" "$ROOT/src/mvk_log_config.c" \
  "$ROOT/src/mvk_log_policy.c" \
  "$ROOT/src/mvk_compat.c" \
  "$ROOT/src/eso_fx_sentinel.c" "$ROOT/src/eso_inactive_pacing.c" \
  "$ROOT/src/mvk_lifecycle.c" "$ROOT/src/mvk_reset_trace.c" \
  "$ROOT/src/mvk_render_audit.c" "$ROOT/src/mvk_present_pixel.m" \
  "$ROOT/src/mvk_swapchain_experiment.c" \
  -framework Metal -framework Foundation \
  -Wl,-install_name,@executable_path/libBink2Macx64.dylib \
  -Wl,-reexport_library,"$BUILD/libBink2Macx64.teso4m4-original.dylib" \
  -o "$BUILD/libBink2Macx64.dylib"

xcrun clang -arch arm64 -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -O2 -I"$BUILD" \
  "$ROOT/tools/compat_audit.c" -o "$BUILD/eso-compat-audit"

xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  "$ROOT/tools/smoke_proxy.c" -o "$BUILD/smoke_proxy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  "$ROOT/tools/probe_self_patch.c" -o "$BUILD/probe_self_patch"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  -I"$ROOT/src" "$ROOT/tools/probe_fx_sentinel.c" \
  "$ROOT/src/eso_fx_sentinel.c" -o "$BUILD/probe_fx_sentinel"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  -I"$ROOT/src" "$ROOT/tools/probe_inactive_pacing.c" \
  "$ROOT/src/eso_inactive_pacing.c" -o "$BUILD/probe_inactive_pacing"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" "$ROOT/tools/probe_log_policy.c" \
  "$ROOT/src/mvk_log_policy.c" -o "$BUILD/probe_log_policy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" "$ROOT/tools/probe_log_file.c" \
  "$ROOT/src/mvk_log_file.c" -o "$BUILD/probe_log_file"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" "$ROOT/tools/probe_log_config.c" \
  "$ROOT/src/mvk_log_config.c" -o "$BUILD/probe_log_config"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" -o "$BUILD/probe_vulkan"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_mvk_config.c" -o "$BUILD/probe_mvk_config"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -DTESO4M4_STATIC_MOLTENVK=1 -I"$ROOT/src" \
  -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" "$LEGACY_MVK" \
  -framework Metal -framework Foundation -framework QuartzCore -framework IOSurface \
  -framework IOKit -framework CoreGraphics -framework AppKit -lc++ \
  -o "$BUILD/probe_vulkan_legacy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_hdr_filter.c" "$ROOT/src/mvk_compat.c" \
  -o "$BUILD/probe_hdr_filter"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_lifecycle.c" "$ROOT/src/mvk_lifecycle.c" \
  -o "$BUILD/probe_lifecycle"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O2 \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_swapchain_experiment.c" \
  "$ROOT/src/mvk_swapchain_experiment.c" \
  -o "$BUILD/probe_swapchain_experiment"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_reset_trace.c" "$ROOT/src/mvk_reset_trace.c" \
  "$ROOT/src/mvk_lifecycle.c" "$ROOT/src/mvk_render_audit.c" \
  -o "$BUILD/probe_reset_trace"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_render_audit.c" "$ROOT/src/mvk_render_audit.c" \
  -o "$BUILD/probe_render_audit"
xcrun clang -fobjc-arc -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_surface_formats.m" "$ROOT/src/mvk_compat.c" \
  -framework AppKit -framework QuartzCore -o "$BUILD/probe_surface_formats"
xcrun clang -fobjc-arc -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -DTESO4M4_STATIC_MOLTENVK=1 \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_surface_formats.m" "$ROOT/src/mvk_compat.c" "$LEGACY_MVK" \
  -framework Metal -framework Foundation -framework QuartzCore -framework IOSurface \
  -framework IOKit -framework CoreGraphics -framework AppKit -lc++ \
  -o "$BUILD/probe_surface_formats_legacy"

# Every probe that loads the bridge runs its constructor. Redirect those
# non-game run records to a temporary directory so a build never appends a
# run to the player's production log, and prove the production log is
# untouched afterwards.
PRODUCTION_LOG="$HOME/Library/Logs/ESO MoltenVK Patcher/bridge.log"
production_log_identity() {
  [[ -e "$PRODUCTION_LOG" ]] || { print -- absent; return 0; }
  stat -f '%i %z %m' "$PRODUCTION_LOG"
}
PRODUCTION_LOG_BEFORE="$(production_log_identity)"
PROBE_LOG_DIR="$(mktemp -d "${TMPDIR:-/private/tmp}/teso4m4-build-log.XXXXXX")"
PROBE_LOG_DIR="${PROBE_LOG_DIR:A}"
trap 'rm -rf -- "$PROBE_LOG_DIR"' EXIT
export TESO4M4_LOG_DIR="$PROBE_LOG_DIR"

"$BUILD/smoke_proxy" "$BUILD/libBink2Macx64.dylib"
grep -q '\] RUN_START: bridge starting' "$PROBE_LOG_DIR/bridge.log" \
  && grep -q '\] SKIP: enable marker absent' "$PROBE_LOG_DIR/bridge.log" || {
  echo "Bridge smoke did not write its non-game run to the probe log."
  exit 1
}
echo "Bridge smoke log isolation: PASS (temporary log, production log untouched)"
"$BUILD/probe_self_patch"
"$BUILD/probe_fx_sentinel"
"$BUILD/probe_inactive_pacing"
"$BUILD/probe_log_policy"
"$BUILD/probe_log_file"
"$BUILD/probe_log_config"
"$BUILD/probe_hdr_filter"
"$BUILD/probe_lifecycle"
"$BUILD/probe_swapchain_experiment"
"$BUILD/probe_reset_trace"
"$BUILD/probe_render_audit"
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" performance-aggressive
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-neutralize
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-pipeline-timing-control
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-inactive-pacing-bypass
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-audit-pacing-bypass
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-neutralize-pacing-bypass
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-neutralize-pacing-release
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-release-argument-buffers
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-release-swapchain-control
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-release-triple-buffer
[[ "$(production_log_identity)" == "$PRODUCTION_LOG_BEFORE" ]] || {
  echo "A build probe changed the production bridge log: $PRODUCTION_LOG"
  exit 1
}
echo "Built teso4m4 artifacts in $BUILD"
