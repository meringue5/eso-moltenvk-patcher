#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
OFFICIAL_ROOT="${MVK_OFFICIAL_ROOT:-$ROOT/vendor/MoltenVK}"
CANDIDATE_ROOT="${MVK_CANDIDATE_ROOT:-$ROOT/vendor/MoltenVK-1.4.2-official}"
HEADER_ROOT="${MVK_HEADER_ROOT:-$OFFICIAL_ROOT}"
SOURCE="$ROOT/tools/probe_reset_composite.mm"
VERTEX_ASM="$ROOT/tools/shaders/reset_composite.vert.spvasm"
FRAGMENT_ASM="$ROOT/tools/shaders/reset_composite.frag.spvasm"
SPIRV_AS="${SPIRV_AS:-$ROOT/vendor/SPIRV-Tools/bin/spirv-as}"
BUILD="$ROOT/build"
OFFICIAL_PROBE="$BUILD/probe_reset_composite_official"
CANDIDATE_PROBE="$BUILD/probe_reset_composite_candidate"
VERTEX_SPV="$BUILD/reset_composite.vert.spv"
FRAGMENT_SPV="$BUILD/reset_composite.frag.spv"
LIVE_CHECK="${TESO4M4_RESET_LIVE_CHECK:-1}"
SYNCHRONOUS_SUBMITS="${TESO4M4_RESET_SYNCHRONOUS_SUBMITS:-1}"
CONCURRENT_COMPILATION="${TESO4M4_RESET_CONCURRENT_COMPILATION:-0}"

for value in "$LIVE_CHECK" "$SYNCHRONOUS_SUBMITS" \
  "$CONCURRENT_COMPILATION"; do
  [[ "$value" == 0 || "$value" == 1 ]] || {
    echo "Reset-composite configuration values must be 0 or 1."
    exit 2
  }
done

for file in \
  "$OFFICIAL_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib" \
  "$CANDIDATE_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib" \
  "$SOURCE" "$VERTEX_ASM" "$FRAGMENT_ASM" "$SPIRV_AS"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
[[ -x "$SPIRV_AS" ]] || {
  echo "SPIR-V assembler is not executable: $SPIRV_AS"
  exit 1
}
mkdir -p "$BUILD"

"$SPIRV_AS" --target-env vulkan1.0 "$VERTEX_ASM" -o "$VERTEX_SPV"
"$SPIRV_AS" --target-env vulkan1.0 "$FRAGMENT_ASM" -o "$FRAGMENT_SPV"

build_probe() {
  local runtime_root="$1"
  local output="$2"
  local dylib_directory="$runtime_root/MoltenVK/dynamic/dylib/macOS"
  xcrun clang++ -fobjc-arc -std=c++17 -arch x86_64 \
    -mmacosx-version-min=12.0 -Wall -Wextra -Werror \
    -I"$HEADER_ROOT/MoltenVK/include" \
    "$SOURCE" -L"$dylib_directory" -lMoltenVK \
    -Wl,-rpath,"$dylib_directory" \
    -framework Foundation -framework Metal \
    -o "$output"
}

build_probe "$OFFICIAL_ROOT" "$OFFICIAL_PROBE"
build_probe "$CANDIDATE_ROOT" "$CANDIDATE_PROBE"

MVK_CONFIG_LOG_LEVEL=0 \
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES="$LIVE_CHECK" \
MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS="$SYNCHRONOUS_SUBMITS" \
MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION="$CONCURRENT_COMPILATION" \
  "$OFFICIAL_PROBE" "$VERTEX_SPV" "$FRAGMENT_SPV"

MVK_CONFIG_LOG_LEVEL=0 \
MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES="$LIVE_CHECK" \
MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS="$SYNCHRONOUS_SUBMITS" \
MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION="$CONCURRENT_COMPILATION" \
  "$CANDIDATE_PROBE" "$VERTEX_SPV" "$FRAGMENT_SPV"

echo "Reset composite differential probe: PASS"
echo "Official MoltenVK 1.4.1: PASS"
echo "Official MoltenVK 1.4.2: PASS"
