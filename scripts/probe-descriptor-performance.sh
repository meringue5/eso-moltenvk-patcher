#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
MVK_ROOT="${MVK_ROOT:-$ROOT/vendor/MoltenVK}"
HEADER_ROOT="${MVK_HEADER_ROOT:-$MVK_ROOT}"
MVK_DYLIB_DIR="$MVK_ROOT/MoltenVK/dynamic/dylib/macOS"
SOURCE="$ROOT/tools/probe_reset_composite.mm"
VERTEX_ASM="$ROOT/tools/shaders/reset_composite.vert.spvasm"
FRAGMENT_ASM="$ROOT/tools/shaders/reset_composite.frag.spvasm"
SPIRV_AS="${SPIRV_AS:-$ROOT/vendor/SPIRV-Tools/bin/spirv-as}"
BUILD="$ROOT/build"
PROBE="$BUILD/probe_descriptor_encode"
VERTEX_SPV="$BUILD/reset_composite.vert.spv"
FRAGMENT_SPV="$BUILD/reset_composite.frag.spv"
OUTPUT="${1:-$BUILD/descriptor-performance}"
DRAWS="${TESO4M4_DESCRIPTOR_BENCHMARK_DRAWS:-20000}"

for file in \
  "$MVK_DYLIB_DIR/libMoltenVK.dylib" \
  "$SOURCE" "$VERTEX_ASM" "$FRAGMENT_ASM" "$SPIRV_AS"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
[[ -x "$SPIRV_AS" ]] || {
  echo "SPIR-V assembler is not executable: $SPIRV_AS"
  exit 1
}
if [[ "$DRAWS" != <-> ]] || (( DRAWS < 2 || DRAWS > 100000 )); then
  echo "TESO4M4_DESCRIPTOR_BENCHMARK_DRAWS must be 2..100000"
  exit 2
fi
[[ ! -e "$OUTPUT" ]] || {
  echo "Output directory already exists: $OUTPUT"
  exit 1
}
mkdir -p "$BUILD" "$OUTPUT"

"$SPIRV_AS" --target-env vulkan1.0 "$VERTEX_ASM" -o "$VERTEX_SPV"
"$SPIRV_AS" --target-env vulkan1.0 "$FRAGMENT_ASM" -o "$FRAGMENT_SPV"
xcrun clang++ -fobjc-arc -std=c++17 -arch x86_64 \
  -mmacosx-version-min=12.0 -Wall -Wextra -Werror \
  -I"$HEADER_ROOT/MoltenVK/include" \
  "$SOURCE" -L"$MVK_DYLIB_DIR" -lMoltenVK \
  -Wl,-rpath,"$MVK_DYLIB_DIR" \
  -framework Foundation -framework Metal \
  -o "$PROBE"

run_probe() {
  local live_check="$1"
  local output_file="$2"
  MVK_CONFIG_LOG_LEVEL=0 \
  MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES="$live_check" \
  MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
  MVK_CONFIG_USE_MTLHEAP=1 \
  MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=1 \
  MVK_CONFIG_USE_COMMAND_POOLING=1 \
  MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0 \
  TESO4M4_DESCRIPTOR_BENCHMARK_DRAWS="$DRAWS" \
    "$PROBE" "$VERTEX_SPV" "$FRAGMENT_SPV" > "$output_file"
}

run_probe 1 "$OUTPUT/live-on-1.txt"
run_probe 0 "$OUTPUT/live-off-1.txt"
run_probe 0 "$OUTPUT/live-off-2.txt"
run_probe 1 "$OUTPUT/live-on-2.txt"
run_probe 1 "$OUTPUT/live-on-3.txt"
run_probe 0 "$OUTPUT/live-off-3.txt"

python3 "$ROOT/tools/analyze_descriptor_benchmark.py" \
  --live-on "$OUTPUT/live-on-1.txt" \
  --live-on "$OUTPUT/live-on-2.txt" \
  --live-on "$OUTPUT/live-on-3.txt" \
  --live-off "$OUTPUT/live-off-1.txt" \
  --live-off "$OUTPUT/live-off-2.txt" \
  --live-off "$OUTPUT/live-off-3.txt" \
  > "$OUTPUT/analysis.txt"
cat "$OUTPUT/analysis.txt"
echo "Descriptor performance probe output: $OUTPUT"
