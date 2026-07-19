#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
VERSION="1.4.1"
ARCHIVE="$ROOT/vendor/MoltenVK-macos-v$VERSION.tar"
EXPECTED_SHA="5ea0c259df7ded9a275444820f09cced54d6e5a7c7a31d262de62a5cdb7e15cf"
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

tar -xf "$ARCHIVE" -C "$ROOT/vendor"
echo "Fetched and verified MoltenVK $VERSION."

