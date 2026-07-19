#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
GAME_MAC="$ESO_APP/Contents/MacOS"
ESO="$GAME_MAC/eso"
BINK="$GAME_MAC/libBink2Macx64.dylib"
MVK_ROOT="${MVK_ROOT:-$ROOT/vendor/MoltenVK}"
MVK="$MVK_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
MANIFEST="${TESO4M4_TARGET_MANIFEST:-$ROOT/config/targets-eso-2026-07-11.json}"
BUILD="$ROOT/build"

for file in "$ESO" "$BINK" "$MVK" "$MANIFEST"; do
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
  -Wall -Wextra -Werror -O2 -I"$BUILD" \
  "$ROOT/src/mvk_shim.c" \
  -Wl,-install_name,@executable_path/libBink2Macx64.dylib \
  -Wl,-reexport_library,"$BUILD/libBink2Macx64.teso4m4-original.dylib" \
  -o "$BUILD/libBink2Macx64.dylib"

xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  "$ROOT/tools/smoke_proxy.c" -o "$BUILD/smoke_proxy"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror -O0 \
  "$ROOT/tools/probe_self_patch.c" -o "$BUILD/probe_self_patch"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
  -I"$MVK_ROOT/MoltenVK/include" "$ROOT/tools/probe_vulkan.c" -o "$BUILD/probe_vulkan"

"$BUILD/smoke_proxy" "$BUILD/libBink2Macx64.dylib"
"$BUILD/probe_self_patch"
echo "Built teso4m4 artifacts in $BUILD"

