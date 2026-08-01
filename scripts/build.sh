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
SOURCE_BINK="$BINK"
if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  [[ -f "$PRISTINE" ]] || {
    echo "Active Bink is a bridge and the pristine build source is missing."
    exit 1
  }
  SOURCE_BINK="$PRISTINE"
elif [[ -f "$PRISTINE" ]]; then
  cmp -s "$BINK" "$PRISTINE" || {
    echo "Active original Bink differs from the pristine build source."
    exit 1
  }
fi

mkdir -p "$BUILD"
python3 "$ROOT/tools/generate_targets.py" "$ESO" "$MANIFEST" "$BUILD/generated_targets.h"

cp -p "$SOURCE_BINK" "$BUILD/libBink2Macx64.teso4m4-original.dylib"
install_name_tool -id @loader_path/libBink2Macx64.teso4m4-original.dylib \
  "$BUILD/libBink2Macx64.teso4m4-original.dylib"
cp -p "$MVK" "$BUILD/libMoltenVK.teso4m4.dylib"

xcrun clang -fobjc-arc -dynamiclib -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -O2 -I"$BUILD" -I"$ROOT/src" \
  -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/src/mvk_shim.c" "$ROOT/src/mvk_compat.c" \
  "$ROOT/src/eso_fx_sentinel.c" \
  "$ROOT/src/mvk_lifecycle.c" "$ROOT/src/mvk_reset_trace.c" \
  "$ROOT/src/mvk_render_audit.c" "$ROOT/src/mvk_present_pixel.m" \
  -framework Metal -framework Foundation \
  -Wl,-install_name,@executable_path/libBink2Macx64.dylib \
  -Wl,-reexport_library,"$BUILD/libBink2Macx64.teso4m4-original.dylib" \
  -o "$BUILD/libBink2Macx64.dylib"

xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  "$ROOT/tools/smoke_proxy.c" -o "$BUILD/smoke_proxy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  "$ROOT/tools/probe_self_patch.c" -o "$BUILD/probe_self_patch"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  -I"$ROOT/src" "$ROOT/tools/probe_fx_sentinel.c" \
  "$ROOT/src/eso_fx_sentinel.c" -o "$BUILD/probe_fx_sentinel"
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
  "$ROOT/tools/probe_legacy_feature_profile.c" "$ROOT/src/mvk_compat.c" \
  -o "$BUILD/probe_legacy_feature_profile"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_INCLUDE_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_lifecycle.c" "$ROOT/src/mvk_lifecycle.c" \
  -o "$BUILD/probe_lifecycle"
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

"$BUILD/smoke_proxy" "$BUILD/libBink2Macx64.dylib"
"$BUILD/probe_self_patch"
"$BUILD/probe_fx_sentinel"
"$BUILD/probe_hdr_filter"
"$BUILD/probe_legacy_feature_profile"
"$BUILD/probe_lifecycle"
"$BUILD/probe_reset_trace"
"$BUILD/probe_render_audit"
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" default
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" descriptor-compat
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" legacy-allocation
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" reset-resource-trace
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" no-command-pooling
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" render-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" reset-no-pipeline-cache
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" full-lifetime-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" texture-cache-fix
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" legacy-feature-profile
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" performance-safe
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" performance-aggressive
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-color-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-fx-neutralize
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-present-pixel-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-draw-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-input-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-audit
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" startup-compositor-neutralize
echo "Built teso4m4 artifacts in $BUILD"
