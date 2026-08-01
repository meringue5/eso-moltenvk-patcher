#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
VERSION="${1:-0.1.0-dev}"
NAME="ESO MoltenVK Patcher"
STAGE="$ROOT/dist/stage"
APP="$STAGE/$NAME.app"
PAYLOAD="$APP/Contents/Resources/Payload"
OUTPUT="$ROOT/dist/ESO-MoltenVK-Patcher-$VERSION.dmg"
SWIFT_CACHE="$ROOT/build/swift-module-cache"

"$ROOT/scripts/build.sh"
rm -rf "$STAGE"
mkdir -p "$PAYLOAD" "$APP/Contents/MacOS"
mkdir -p "$SWIFT_CACHE"
cp "$ROOT/app/Info.plist" "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION" "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSION" "$APP/Contents/Info.plist"
CLANG_MODULE_CACHE_PATH="$SWIFT_CACHE" SWIFT_MODULE_CACHE_PATH="$SWIFT_CACHE" \
  xcrun swiftc -parse-as-library -O -framework AppKit -framework CryptoKit \
  "$ROOT/app/ESO MoltenVK Patcher.swift" \
  -o "$APP/Contents/MacOS/$NAME"
cp "$ROOT/build/libBink2Macx64.dylib" "$PAYLOAD/"
cp "$ROOT/build/libMoltenVK.teso4m4.dylib" "$PAYLOAD/"
TARGET_NAME="$(<"$ROOT/config/current-target.txt")"
cp "$ROOT/config/$TARGET_NAME" "$PAYLOAD/target-profile.json"

if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
  codesign --force --options runtime --timestamp --sign "$CODESIGN_IDENTITY" \
    "$APP"
else
  codesign --force --sign - "$APP"
fi
codesign --verify --deep --strict "$APP"
rm -f "$OUTPUT"
hdiutil create -quiet -volname "$NAME" -srcfolder "$STAGE" -ov -format UDZO "$OUTPUT"
echo "Created $OUTPUT"
