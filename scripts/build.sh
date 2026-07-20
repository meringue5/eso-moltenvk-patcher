#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
source "$ROOT/scripts/lib-target.sh"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
LEGACY_MVK="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"
MVK_ROOT="${MVK_ROOT:-$ROOT/vendor/MoltenVK}"
MVK="$MVK_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
MANIFEST="$(teso4m4_resolve_target_manifest "$ROOT")"
BUILD="$ROOT/build"

for file in "$ESO" "$BINK" "$LEGACY_MVK" "$MVK" "$MANIFEST"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
if otool -L "$BINK" | grep -q 'teso4m4-original'; then
  echo "Active Bink is already a teso4m4 proxy. Restore before rebuilding."
  exit 1
fi

mkdir -p "$BUILD"
python3 "$ROOT/tools/generate_targets.py" "$ESO" "$MANIFEST" "$BUILD/generated_targets.h"

cp -p "$BINK" "$BUILD/libBink2Macx64.teso4m4-original.dylib"
install_name_tool -id @loader_path/libBink2Macx64.teso4m4-original.dylib \
  "$BUILD/libBink2Macx64.teso4m4-original.dylib"
cp -p "$MVK" "$BUILD/libMoltenVK.teso4m4.dylib"

xcrun clang -dynamiclib -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -O2 -I"$BUILD" -I"$ROOT/src" \
  -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/src/mvk_shim.c" "$ROOT/src/mvk_compat.c" \
  -Wl,-install_name,@executable_path/libBink2Macx64.dylib \
  -Wl,-reexport_library,"$BUILD/libBink2Macx64.teso4m4-original.dylib" \
  -o "$BUILD/libBink2Macx64.dylib"

xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  "$ROOT/tools/smoke_proxy.c" -o "$BUILD/smoke_proxy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  "$ROOT/tools/probe_self_patch.c" -o "$BUILD/probe_self_patch"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" -o "$BUILD/probe_vulkan"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_mvk_config.c" -o "$BUILD/probe_mvk_config"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -DTESO4M4_STATIC_MOLTENVK=1 -I"$ROOT/src" \
  -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" "$LEGACY_MVK" \
  -framework Metal -framework Foundation -framework QuartzCore -framework IOSurface \
  -framework IOKit -framework CoreGraphics -framework AppKit -lc++ \
  -o "$BUILD/probe_vulkan_legacy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_hdr_filter.c" "$ROOT/src/mvk_compat.c" \
  -o "$BUILD/probe_hdr_filter"
xcrun clang -fobjc-arc -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -I"$ROOT/src" -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_surface_formats.m" "$ROOT/src/mvk_compat.c" \
  -framework AppKit -framework QuartzCore -o "$BUILD/probe_surface_formats"
xcrun clang -fobjc-arc -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -DTESO4M4_STATIC_MOLTENVK=1 \
  -I"$ROOT/src" -I"$MVK_ROOT/MoltenVK/include" \
  "$ROOT/tools/probe_surface_formats.m" "$ROOT/src/mvk_compat.c" "$LEGACY_MVK" \
  -framework Metal -framework Foundation -framework QuartzCore -framework IOSurface \
  -framework IOKit -framework CoreGraphics -framework AppKit -lc++ \
  -o "$BUILD/probe_surface_formats_legacy"

"$BUILD/smoke_proxy" "$BUILD/libBink2Macx64.dylib"
"$BUILD/probe_self_patch"
"$BUILD/probe_hdr_filter"
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" default
"$BUILD/probe_mvk_config" "$BUILD/libMoltenVK.teso4m4.dylib" descriptor-compat
echo "Built teso4m4 artifacts in $BUILD"
