#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
RUNTIME_ROOT="${MVK_RUNTIME_ROOT:-$ROOT/vendor/MoltenVK-1.4.2-official}"
HEADER_ROOT="${MVK_HEADER_ROOT:-$RUNTIME_ROOT}"
DYLIB_DIRECTORY="$RUNTIME_ROOT/MoltenVK/dynamic/dylib/macOS"
DYLIB="$DYLIB_DIRECTORY/libMoltenVK.dylib"
SOURCE="$ROOT/tools/probe_startup_surface.mm"
OUTPUT="$ROOT/build/probe_startup_surface"
LIFECYCLE_OBJECT="$ROOT/build/probe_startup_surface_lifecycle.o"
PRESENT_PIXEL_OBJECT="$ROOT/build/probe_startup_surface_present_pixel.o"

for file in "$DYLIB" "$SOURCE"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
mkdir -p "$ROOT/build"

xcrun clang -std=c11 -arch x86_64 -mmacosx-version-min=12.0 \
  -Wall -Wextra -Werror -I"$ROOT/src" \
  -I"$HEADER_ROOT/MoltenVK/include" \
  -c "$ROOT/src/mvk_lifecycle.c" -o "$LIFECYCLE_OBJECT"

xcrun clang -fobjc-arc -arch x86_64 -mmacosx-version-min=12.0 \
  -Wall -Wextra -Werror -I"$ROOT/src" \
  -I"$HEADER_ROOT/MoltenVK/include" \
  -c "$ROOT/src/mvk_present_pixel.m" -o "$PRESENT_PIXEL_OBJECT"

xcrun clang++ -fobjc-arc -std=c++17 -arch x86_64 \
  -mmacosx-version-min=12.0 -Wall -Wextra -Werror \
  -I"$ROOT/src" -I"$HEADER_ROOT/MoltenVK/include" \
  "$SOURCE" "$LIFECYCLE_OBJECT" "$PRESENT_PIXEL_OBJECT" \
  -L"$DYLIB_DIRECTORY" -lMoltenVK \
  -Wl,-rpath,"$DYLIB_DIRECTORY" \
  -framework AppKit -framework Metal -framework QuartzCore \
  -o "$OUTPUT"

run_probe() {
  MVK_CONFIG_LOG_LEVEL=0 \
  MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=0 \
  MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
  MVK_CONFIG_USE_MTLHEAP=1 \
  MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0 \
  MVK_CONFIG_USE_COMMAND_POOLING=1 \
  MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0 \
  MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1 \
    "$OUTPUT" "$@"
}

run_probe neon-pink
run_probe black
for trial in 1 2 3 4 5; do
  echo "load-only trial=$trial"
  run_probe load
done

run_probe black 3420 2146
run_probe load 3420 2146

echo "Startup surface non-game probe: PASS"
