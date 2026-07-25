#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
SOURCE_COMMIT="db445ff2042d9ce348c439ad8451112f354b8d2a"
SOURCE_SHORT_REVISION="db445ff"
UPSTREAM_FIX_COMMIT="9a5e233ef08e3a0f58c7b90053385cfb5cacde68"
PATCH="$ROOT/patches/moltenvk-1.4.1/0001-refresh-swapchain-image-view-texture.patch"
PATCH_SHA="4d31f4ce6175935e2208061e800c57ddf2c47679ca6d91384768ae7527386686"
SOURCE_DIR="${MVK_SOURCE_DIR:-$ROOT/vendor/MoltenVK-source-v1.4.1}"
BUILD_DIR="${MVK_SOURCE_BUILD_DIR:-$ROOT/vendor/MoltenVK-build-v1.4.1-texture-cache-fix}"
CPM_CACHE="${MVK_CPM_CACHE:-$ROOT/vendor/MoltenVK-cpm-cache}"
UV_CACHE="${MVK_UV_CACHE:-$ROOT/vendor/uv-cache-moltenvk}"
OUTPUT_ROOT="${MVK_OUTPUT_ROOT:-$ROOT/vendor/MoltenVK-1.4.1-texture-cache-fix}"
OUTPUT_DYLIB="$OUTPUT_ROOT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
JOBS="${MVK_BUILD_JOBS:-8}"

run_cmake() {
  if [[ -n "${MVK_CMAKE:-}" ]]; then
    [[ -x "$MVK_CMAKE" ]] || {
      echo "MVK_CMAKE is not executable: $MVK_CMAKE"
      return 1
    }
    "$MVK_CMAKE" "$@"
  elif command -v cmake >/dev/null; then
    cmake "$@"
  else
    command -v uv >/dev/null || {
      echo "cmake or uv is required for the source build."
      return 1
    }
    UV_CACHE_DIR="$UV_CACHE" uv run --with cmake cmake "$@"
  fi
}
[[ -f "$PATCH" ]] || { echo "Missing patch: $PATCH"; exit 1; }
actual_patch_sha="$(shasum -a 256 "$PATCH" | awk '{print $1}')"
[[ "$actual_patch_sha" == "$PATCH_SHA" ]] || {
  echo "MoltenVK patch SHA-256 mismatch."
  echo "Expected: $PATCH_SHA"
  echo "Actual:   $actual_patch_sha"
  exit 1
}
[[ "$JOBS" == <-> && "$JOBS" -gt 0 ]] || {
  echo "MVK_BUILD_JOBS must be a positive integer."
  exit 1
}

if [[ ! -d "$SOURCE_DIR/.git" ]]; then
  [[ ! -e "$SOURCE_DIR" ]] || {
    echo "Refusing non-repository source path: $SOURCE_DIR"
    exit 1
  }
  mkdir -p "${SOURCE_DIR:h}"
  git clone https://github.com/KhronosGroup/MoltenVK.git "$SOURCE_DIR"
  git -C "$SOURCE_DIR" checkout --detach "$SOURCE_COMMIT"
fi

origin="$(git -C "$SOURCE_DIR" remote get-url origin)"
[[ "$origin" == "https://github.com/KhronosGroup/MoltenVK.git" ]] || {
  echo "Unexpected MoltenVK origin: $origin"
  exit 1
}
head_commit="$(git -C "$SOURCE_DIR" rev-parse HEAD)"
[[ "$head_commit" == "$SOURCE_COMMIT" ]] || {
  echo "Unexpected MoltenVK source commit."
  echo "Expected: $SOURCE_COMMIT"
  echo "Actual:   $head_commit"
  exit 1
}
git -C "$SOURCE_DIR" cat-file -e "$UPSTREAM_FIX_COMMIT^{commit}"
upstream_patch_sha="$(
  git -C "$SOURCE_DIR" show --format= --no-ext-diff \
    "$UPSTREAM_FIX_COMMIT" -- \
    MoltenVK/MoltenVK/GPUObjects/MVKImage.h \
    MoltenVK/MoltenVK/GPUObjects/MVKImage.mm \
    | shasum -a 256 | awk '{print $1}'
)"
[[ "$upstream_patch_sha" == "$PATCH_SHA" ]] || {
  echo "Committed patch is not the exact upstream fix."
  echo "Expected: $PATCH_SHA"
  echo "Actual:   $upstream_patch_sha"
  exit 1
}

tracked_status="$(git -C "$SOURCE_DIR" status --short --untracked-files=no)"
if [[ -z "$tracked_status" ]]; then
  git -C "$SOURCE_DIR" apply --check "$PATCH"
  git -C "$SOURCE_DIR" apply "$PATCH"
else
  git -C "$SOURCE_DIR" diff --check
  applied_patch_sha="$(
    git -C "$SOURCE_DIR" diff -- \
      MoltenVK/MoltenVK/GPUObjects/MVKImage.h \
      MoltenVK/MoltenVK/GPUObjects/MVKImage.mm \
      | shasum -a 256 | awk '{print $1}'
  )"
  [[ "$applied_patch_sha" == "$PATCH_SHA" ]] || {
    echo "MoltenVK source has unrelated tracked changes; refusing."
    git -C "$SOURCE_DIR" status --short
    exit 1
  }
fi

mkdir -p "$BUILD_DIR" "$CPM_CACHE" "$UV_CACHE"
(
  cd "$SOURCE_DIR"
  run_cmake \
    -S "$SOURCE_DIR" -B "$BUILD_DIR" -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCPM_SOURCE_CACHE="$CPM_CACHE"
  run_cmake \
    --build "$BUILD_DIR" --config Release --parallel "$JOBS"
)

DERIVED_REVISION_HEADER="$BUILD_DIR/MoltenVK/mvkGitRevDerived.h"
[[ -f "$DERIVED_REVISION_HEADER" ]] || {
  echo "MoltenVK build did not generate its revision header."
  exit 1
}
grep -Fq "\"$SOURCE_SHORT_REVISION\"" "$DERIVED_REVISION_HEADER" || {
  echo "MoltenVK embedded an unexpected pipeline-cache revision."
  sed -n '1,8p' "$DERIVED_REVISION_HEADER"
  exit 1
}

BUILT_DYLIB="$BUILD_DIR/MoltenVK/libMoltenVK.dylib"
[[ -f "$BUILT_DYLIB" ]] || {
  echo "Patched MoltenVK build did not produce a dylib."
  exit 1
}
lipo "$BUILT_DYLIB" -verify_arch x86_64
otool -L "$BUILT_DYLIB" | grep -Fq "current version 1.4.1" || {
  echo "Patched MoltenVK does not report version 1.4.1."
  exit 1
}

mkdir -p "${OUTPUT_DYLIB:h}" "$OUTPUT_ROOT/MoltenVK"
cp -p "$BUILT_DYLIB" "$OUTPUT_DYLIB.installing"
install_name_tool -id @rpath/libMoltenVK.dylib "$OUTPUT_DYLIB.installing"
mv -f "$OUTPUT_DYLIB.installing" "$OUTPUT_DYLIB"

binary_sha="$(shasum -a 256 "$OUTPUT_DYLIB" | awk '{print $1}')"
{
  echo "MoltenVK version: 1.4.1"
  echo "Source: https://github.com/KhronosGroup/MoltenVK.git"
  echo "Source commit: $SOURCE_COMMIT"
  echo "Upstream fix commit: $UPSTREAM_FIX_COMMIT"
  echo "Patch SHA-256: $PATCH_SHA"
  echo "Embedded pipeline-cache revision: $SOURCE_SHORT_REVISION"
  echo "Architecture: x86_64"
  echo "Binary SHA-256: $binary_sha"
} > "$OUTPUT_ROOT/PROVENANCE.txt.installing"
mv -f "$OUTPUT_ROOT/PROVENANCE.txt.installing" \
  "$OUTPUT_ROOT/PROVENANCE.txt"

echo "Built exact MoltenVK 1.4.1 texture-cache backport."
echo "MVK_ROOT=$OUTPUT_ROOT"
echo "Binary SHA-256: $binary_sha"
