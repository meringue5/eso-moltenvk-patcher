#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
OFFICIAL_ROOT="${MVK_OFFICIAL_ROOT:-$ROOT/vendor/MoltenVK}"
CANDIDATE_ROOT="${MVK_CANDIDATE_ROOT:-$ROOT/vendor/MoltenVK-1.4.1-texture-cache-fix}"
CANDIDATE_HEADER_ROOT="${MVK_CANDIDATE_HEADER_ROOT:-$OFFICIAL_ROOT}"
OFFICIAL_DYLIB="$OFFICIAL_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
CANDIDATE_DYLIB="$CANDIDATE_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
SOURCE="$ROOT/tools/probe_swapchain_texture_cache.mm"
BUILD="$ROOT/build"
OFFICIAL_PROBE="$BUILD/probe_swapchain_texture_cache_official"
CANDIDATE_PROBE="$BUILD/probe_swapchain_texture_cache_candidate"
OFFICIAL_IDENTITY_PROBE="$BUILD/probe_swapchain_texture_cache_official_identity"
CANDIDATE_IDENTITY_PROBE="$BUILD/probe_swapchain_texture_cache_candidate_identity"

for file in "$OFFICIAL_DYLIB" "$CANDIDATE_DYLIB" "$SOURCE"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
mkdir -p "$BUILD"

build_probe() {
  local runtime_root="$1"
  local header_root="$2"
  local output="$3"
  local identity_view="$4"
  local dylib_directory="$runtime_root/MoltenVK/dynamic/dylib/macOS"
  xcrun clang++ -fobjc-arc -std=c++17 -arch x86_64 \
    -mmacosx-version-min=11.0 -Wall -Wextra -Werror \
    -DTESO4M4_PROBE_IDENTITY_VIEW="$identity_view" \
    -I"$header_root/MoltenVK/include" \
    "$SOURCE" -L"$dylib_directory" -lMoltenVK \
    -Wl,-rpath,"$dylib_directory" \
    -framework AppKit -framework Metal -framework QuartzCore \
    -o "$output"
}

build_probe "$OFFICIAL_ROOT" "$OFFICIAL_ROOT" "$OFFICIAL_PROBE" 0
build_probe "$CANDIDATE_ROOT" "$CANDIDATE_HEADER_ROOT" "$CANDIDATE_PROBE" 0
build_probe "$OFFICIAL_ROOT" "$OFFICIAL_ROOT" "$OFFICIAL_IDENTITY_PROBE" 1
build_probe \
  "$CANDIDATE_ROOT" "$CANDIDATE_HEADER_ROOT" "$CANDIDATE_IDENTITY_PROBE" 1

official_exit=0
MVK_CONFIG_LOG_LEVEL=0 "$OFFICIAL_PROBE" || official_exit=$?
[[ "$official_exit" -eq 3 ]] || {
  echo "Official MoltenVK 1.4.1 did not reproduce the stale-view defect."
  echo "Expected exit: 3"
  echo "Actual exit:   $official_exit"
  exit 1
}

candidate_exit=0
MVK_CONFIG_LOG_LEVEL=0 "$CANDIDATE_PROBE" || candidate_exit=$?
[[ "$candidate_exit" -eq 0 ]] || {
  echo "Patched MoltenVK 1.4.1 did not repair the stale-view defect."
  echo "Expected exit: 0"
  echo "Actual exit:   $candidate_exit"
  exit 1
}

official_identity_exit=0
MVK_CONFIG_LOG_LEVEL=0 "$OFFICIAL_IDENTITY_PROBE" ||
  official_identity_exit=$?
[[ "$official_identity_exit" -eq 0 ]] || {
  echo "Official MoltenVK 1.4.1 failed the direct identity-view path."
  echo "Expected exit: 0"
  echo "Actual exit:   $official_identity_exit"
  exit 1
}

candidate_identity_exit=0
MVK_CONFIG_LOG_LEVEL=0 "$CANDIDATE_IDENTITY_PROBE" ||
  candidate_identity_exit=$?
[[ "$candidate_identity_exit" -eq 0 ]] || {
  echo "Patched MoltenVK 1.4.1 failed the direct identity-view path."
  echo "Expected exit: 0"
  echo "Actual exit:   $candidate_identity_exit"
  exit 1
}

echo "Swapchain texture-cache differential probe: PASS"
echo "Official MoltenVK 1.4.1 swizzled view: STALE (expected)"
echo "MoltenVK 1.4.1 + exact upstream fix swizzled view: PASS"
echo "Official MoltenVK 1.4.1 identity view: PASS"
echo "MoltenVK 1.4.1 + exact upstream fix identity view: PASS"
