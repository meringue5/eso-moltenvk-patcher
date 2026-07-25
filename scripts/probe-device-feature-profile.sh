#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
ESO_APP="${ESO_APP:-$HOME/Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app}"
LEGACY_MVK="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"
MVK_141_ROOT="${MVK_141_ROOT:-$ROOT/vendor/MoltenVK}"
MVK_142_ROOT="${MVK_142_ROOT:-$ROOT/vendor/MoltenVK-1.4.2}"
MVK_141="$MVK_141_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
MVK_142="$MVK_142_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
HEADERS="$MVK_141_ROOT/MoltenVK/include"
BUILD="$ROOT/build"
PROBE="$BUILD/probe_vulkan_device_profile"
LEGACY_PROBE="$BUILD/probe_vulkan_device_profile_legacy"

for file in "$LEGACY_MVK" "$MVK_141" "$MVK_142" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c"; do
  [[ -f "$file" ]] || { echo "Missing required file: $file"; exit 1; }
done
mkdir -p "$BUILD"

xcrun clang -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -I"$ROOT/src" -I"$HEADERS" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" \
  -o "$PROBE"
xcrun clang -arch x86_64 -mmacosx-version-min=11.0 \
  -Wall -Wextra -Werror -DTESO4M4_STATIC_MOLTENVK=1 \
  -I"$ROOT/src" -I"$HEADERS" \
  "$ROOT/tools/probe_vulkan.c" "$ROOT/src/mvk_compat.c" "$LEGACY_MVK" \
  -framework Metal -framework Foundation -framework QuartzCore \
  -framework IOSurface -framework IOKit -framework CoreGraphics \
  -framework AppKit -lc++ -o "$LEGACY_PROBE"

run_dynamic() {
  local runtime="$1"
  local output="$2"
  shift 2
  env \
    MVK_CONFIG_LOG_LEVEL=0 \
    MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1 \
    MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
    MVK_CONFIG_USE_MTLHEAP=1 \
    MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=1 \
    MVK_CONFIG_USE_COMMAND_POOLING=1 \
    MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0 \
    "$@" "$PROBE" "$runtime" > "$output"
}

PROFILE_1018="$BUILD/device-profile-1.0.18.txt"
PROFILE_141="$BUILD/device-profile-1.4.1.txt"
PROFILE_141_MASKED="$BUILD/device-profile-1.4.1-legacy-mask.txt"
PROFILE_142="$BUILD/device-profile-1.4.2.txt"

MVK_CONFIG_LOG_LEVEL=0 "$LEGACY_PROBE" > "$PROFILE_1018"
run_dynamic "$MVK_141" "$PROFILE_141"
run_dynamic "$MVK_141" "$PROFILE_141_MASKED" \
  TESO4M4_PROBE_LEGACY_FEATURE_PROFILE=1
run_dynamic "$MVK_142" "$PROFILE_142"

rg '^profile feature\.' "$PROFILE_1018" | LC_ALL=C sort \
  > "$BUILD/device-features-1.0.18.txt"
rg '^profile feature\.' "$PROFILE_141" | LC_ALL=C sort \
  > "$BUILD/device-features-1.4.1.txt"
rg '^profile feature\.' "$PROFILE_141_MASKED" | LC_ALL=C sort \
  > "$BUILD/device-features-1.4.1-legacy-mask.txt"

cmp "$BUILD/device-features-1.0.18.txt" \
  "$BUILD/device-features-1.4.1-legacy-mask.txt"

masked_features=(
  robustBufferAccess
  fullDrawIndexUint32
  tessellationShader
  sampleRateShading
  drawIndirectFirstInstance
  multiViewport
  textureCompressionETC2
  textureCompressionASTC_LDR
  shaderTessellationAndGeometryPointSize
  shaderStorageImageReadWithoutFormat
  shaderStorageImageWriteWithoutFormat
  shaderUniformBufferArrayDynamicIndexing
  shaderSampledImageArrayDynamicIndexing
  shaderStorageBufferArrayDynamicIndexing
  shaderStorageImageArrayDynamicIndexing
  shaderInt64
  shaderResourceMinLod
  inheritedQueries
)
for feature in "${masked_features[@]}"; do
  rg -qx "profile feature\.$feature=1" \
    "$BUILD/device-features-1.4.1.txt"
  rg -qx "profile feature\.$feature=0" \
    "$BUILD/device-features-1.0.18.txt"
done
[[ "${#masked_features[@]}" -eq 18 ]]

rg -q \
  '^compat LEGACY_FEATURE_PROFILE: .* raw_enabled=36 visible_enabled=18 masked=18 expected_masked=18$' \
  "$PROFILE_141_MASKED"
rg -q \
  '^compat CREATE_DEVICE_FEATURE_PROFILE: .* enabled=18 prohibited_enabled=0 expected_prohibited=0$' \
  "$PROFILE_141_MASKED"
rg -q '^vkCreateDevice \(ESO-era extension set\): 0$' \
  "$PROFILE_141_MASKED"

for profile in "$PROFILE_141" "$PROFILE_142"; do
  rg '^profile ' "$profile" |
    rg -v '^profile property\.(apiVersion|driverVersion|pipelineCacheUUID)=' |
    LC_ALL=C sort > "$profile.comparable"
done
cmp "$PROFILE_141.comparable" "$PROFILE_142.comparable"

echo "Device feature profile probe: PASS"
echo "Embedded 1.0.18 and masked 1.4.1 features: identical"
echo "Official 1.4.1 raw-only features masked: 18"
echo "MoltenVK 1.4.1 and 1.4.2 core features/limits: identical"
