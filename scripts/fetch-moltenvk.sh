#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
VERSION="1.4.2"
ARCHIVE="$ROOT/vendor/MoltenVK-macos-v$VERSION.tar"
OUTPUT="$ROOT/vendor/MoltenVK-$VERSION-official"
EXPECTED_SHA="f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e"
EXPECTED_DYLIB_SHA="aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f"
URL="https://github.com/KhronosGroup/MoltenVK/releases/download/v$VERSION/MoltenVK-macos.tar"

mkdir -p "$ROOT/vendor"
if [[ ! -f "$ARCHIVE" ]]; then
  curl --fail --location "$URL" --output "$ARCHIVE"
fi

actual_sha="$(shasum -a 256 "$ARCHIVE" | awk '{print $1}')"
if [[ "$actual_sha" != "$EXPECTED_SHA" ]]; then
  echo "MoltenVK archive SHA-256 mismatch."
  echo "Expected: $EXPECTED_SHA"
  echo "Actual:   $actual_sha"
  exit 1
fi

if [[ ! -d "$OUTPUT" ]]; then
  TEMP="$(mktemp -d "$ROOT/vendor/.MoltenVK-$VERSION.XXXXXX")"
  tar -xf "$ARCHIVE" -C "$TEMP"
  [[ -d "$TEMP/MoltenVK" ]] || {
    echo "MoltenVK archive did not contain its expected root."
    exit 1
  }
  mv "$TEMP/MoltenVK" "$OUTPUT"
  rmdir "$TEMP"
fi

DYLIB="$OUTPUT/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib"
[[ -f "$DYLIB" ]] || {
  echo "MoltenVK archive is missing its dynamic macOS library."
  exit 1
}
actual_dylib_sha="$(shasum -a 256 "$DYLIB" | awk '{print $1}')"
if [[ "$actual_dylib_sha" != "$EXPECTED_DYLIB_SHA" ]]; then
  echo "MoltenVK dylib SHA-256 mismatch."
  echo "Expected: $EXPECTED_DYLIB_SHA"
  echo "Actual:   $actual_dylib_sha"
  exit 1
fi

echo "Fetched and verified MoltenVK $VERSION."
echo "MVK_ROOT=$OUTPUT"
