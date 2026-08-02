#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
TEST_ROOT="$(mktemp -d /private/tmp/eso-moltenvk-release-test.XXXXXX)"
trap 'rm -rf "$TEST_ROOT"' EXIT

PACKAGE="$TEST_ROOT/ESO MoltenVK Patcher Test"
PAYLOAD="$PACKAGE/payload"
TOOL="$PACKAGE/bin/eso-moltenvk-patcher"
ESO_APP="$TEST_ROOT/Custom ESO Location/eso.app"
GAME_MAC="$ESO_APP/Contents/MacOS"
STATE_ROOT="$TEST_ROOT/state"
MOCK_BIN="$TEST_ROOT/mock-bin"

mkdir -p "$PACKAGE/bin" "$PAYLOAD" "$GAME_MAC" "$MOCK_BIN"
cp "$ROOT/release/bin/eso-moltenvk-patcher" "$TOOL"
cp "$ROOT/build/libBink2Macx64.dylib" "$PAYLOAD/"
cp "$ROOT/build/libMoltenVK.teso4m4.dylib" "$PAYLOAD/"
cp /usr/bin/true "$GAME_MAC/eso"
cp "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" \
  "$GAME_MAC/libBink2Macx64.dylib"

EXPECTED_ESO_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
EXPECTED_BINK_SHA="$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')"
print -r -- 'RELEASE_VERSION=fixture-1.0.0' > "$PAYLOAD/target-profile.env"
print -r -- "PROFILE_DESCRIPTION='Release installer fixture'" >> "$PAYLOAD/target-profile.env"
print -r -- "EXPECTED_ESO_SHA256=$EXPECTED_ESO_SHA" >> "$PAYLOAD/target-profile.env"
print -r -- "EXPECTED_ORIGINAL_BINK_SHA256=$EXPECTED_BINK_SHA" >> "$PAYLOAD/target-profile.env"
print -r -- 'EXPECTED_ESO_UUID=fixture' >> "$PAYLOAD/target-profile.env"

print -r -- '#!/bin/zsh' > "$MOCK_BIN/pgrep"
print -r -- 'exit 1' >> "$MOCK_BIN/pgrep"
print -r -- '#!/bin/zsh' > "$MOCK_BIN/lsof"
print -r -- 'exit 1' >> "$MOCK_BIN/lsof"
chmod 755 "$TOOL" "$MOCK_BIN/pgrep" "$MOCK_BIN/lsof"

run_tool() {
  PATH="$MOCK_BIN:$PATH" ESO_MOLTENVK_PATCHER_STATE_ROOT="$STATE_ROOT" \
    "$TOOL" "$@" --eso-app "$ESO_APP"
}

run_tool status | grep -q 'patch not installed'
mv "$PAYLOAD/libMoltenVK.teso4m4.dylib" "$PAYLOAD/libMoltenVK.teso4m4.dylib.missing"
if run_tool install >/dev/null 2>&1; then
  print -u2 -- 'Expected the incomplete-payload install to fail.'
  exit 1
fi
mv "$PAYLOAD/libMoltenVK.teso4m4.dylib.missing" "$PAYLOAD/libMoltenVK.teso4m4.dylib"
recovery_output="$(run_tool install)"
print -r -- "$recovery_output" | grep -q 'Restoring the verified baseline before restarting installation.'
print -r -- "$recovery_output" | grep -q 'Installed to:'
run_tool remove >/dev/null
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]

install_output="$(run_tool install)"
print -r -- "$install_output" | grep -q "Installed to: $ESO_APP"
print -r -- "$install_output" | grep -q 'Release: fixture-1.0.0'
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" \
  == "$(shasum -a 256 "$PAYLOAD/libBink2Macx64.dylib" | awk '{print $1}')" ]]
[[ -f "$GAME_MAC/.teso4m4-enable" \
  && -f "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  && -f "$GAME_MAC/libMoltenVK.teso4m4.dylib" ]]
run_tool status | grep -q 'Installed release: fixture-1.0.0'

run_tool remove | grep -q "Removed from: $ESO_APP"
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]
[[ ! -e "$GAME_MAC/.teso4m4-enable" \
  && ! -e "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  && ! -e "$GAME_MAC/libMoltenVK.teso4m4.dylib" ]]

run_tool install >/dev/null
run_tool remove >/dev/null
find "$STATE_ROOT" -name .version -type f -exec grep -q 'fixture-1.0.0' {} \;
print -- 'Release installer transaction: PASS (failed-install recovery, status, install, remove, reinstall)'
