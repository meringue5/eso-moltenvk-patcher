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
SETTINGS_FILE="$TEST_ROOT/Documents/Elder Scrolls Online/live/UserSettings.txt"
LEGACY_MVK="$ESO_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"

mkdir -p "$PACKAGE/bin" "$PAYLOAD" "$GAME_MAC" "$MOCK_BIN"
mkdir -p "${LEGACY_MVK:h}"
mkdir -p "${SETTINGS_FILE:h}"
cp "$ROOT/release/bin/eso-moltenvk-patcher" "$TOOL"
cp "$ROOT/build/libBink2Macx64.dylib" "$PAYLOAD/"
cp "$ROOT/build/libMoltenVK.teso4m4.dylib" "$PAYLOAD/"
cp "$ROOT/config/usersettings-m4-moltenvk-1.4.2-standard.txt" "$PAYLOAD/"
print -r -- 'SET FULLSCREEN "0"' > "$SETTINGS_FILE"
print -r -- 'SET LANGUAGE.2 "en"' >> "$SETTINGS_FILE"
SETTINGS_ORIGINAL_SHA="$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')"
cp /usr/bin/true "$GAME_MAC/eso"
cp "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" \
  "$GAME_MAC/libBink2Macx64.dylib"
cp "$GAME_MAC/libBink2Macx64.dylib" "$TEST_ROOT/original-bink.dylib"
print -r -- 'fixture embedded MoltenVK archive' > "$LEGACY_MVK"
print -r -- '#!/bin/zsh' > "$PAYLOAD/eso-compat-audit"
print -r -- 'exit 0' >> "$PAYLOAD/eso-compat-audit"

EXPECTED_ESO_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
EXPECTED_BINK_SHA="$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')"
print -r -- 'RELEASE_VERSION=fixture-1.0.0' > "$PAYLOAD/target-profile.env"
print -r -- "PROFILE_DESCRIPTION='Release installer fixture'" >> "$PAYLOAD/target-profile.env"
print -r -- "EXPECTED_ESO_SHA256=$EXPECTED_ESO_SHA" >> "$PAYLOAD/target-profile.env"
print -r -- "EXPECTED_ORIGINAL_BINK_SHA256=$EXPECTED_BINK_SHA" >> "$PAYLOAD/target-profile.env"
EXPECTED_RETAGGED_BINK_SHA="$(shasum -a 256 "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" | awk '{print $1}')"
print -r -- "EXPECTED_RETAGGED_ORIGINAL_BINK_SHA256=$EXPECTED_RETAGGED_BINK_SHA" >> "$PAYLOAD/target-profile.env"
print -r -- 'EXPECTED_ESO_UUID=fixture' >> "$PAYLOAD/target-profile.env"
print -r -- "EXPECTED_LEGACY_MVK_ARCHIVE_SHA256=$(shasum -a 256 "$LEGACY_MVK" | awk '{print $1}')" >> "$PAYLOAD/target-profile.env"

print -r -- '#!/bin/zsh' > "$MOCK_BIN/pgrep"
print -r -- 'exit 1' >> "$MOCK_BIN/pgrep"
print -r -- '#!/bin/zsh' > "$MOCK_BIN/lsof"
print -r -- 'exit 1' >> "$MOCK_BIN/lsof"
chmod 755 "$TOOL" "$PAYLOAD/eso-compat-audit" "$MOCK_BIN/pgrep" "$MOCK_BIN/lsof"

cp "$ROOT/build/libBink2Macx64.teso4m4-original.dylib" \
  "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib"
cp "$PAYLOAD/libMoltenVK.teso4m4.dylib" "$GAME_MAC/libMoltenVK.teso4m4.dylib"

run_tool() {
  PATH="$MOCK_BIN:$PATH" ESO_MOLTENVK_PATCHER_STATE_ROOT="$STATE_ROOT" \
    "$TOOL" "$@" --eso-app "$ESO_APP"
}

initial_choice_output="$(run_tool install --yes 2>&1 || true)"
if [[ "$initial_choice_output" != *'Choose settings explicitly'* ]]; then
  print -u2 -- 'Expected non-interactive install without a settings choice to fail.'
  exit 1
fi
[[ "$initial_choice_output" == *'Detected verified inactive development artifacts'* ]]
target_choice_output="$(run_tool install --skip-settings 2>&1 || true)"
[[ "$target_choice_output" == *'Non-interactive installation requires --yes'* ]]
rm -f "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  "$GAME_MAC/libMoltenVK.teso4m4.dylib"
status_output="$(run_tool status)"
[[ "$status_output" == *'patch not installed'* ]]

status_output="$(run_tool status)"
[[ "$status_output" == *'patch not installed'* ]]
mv "$PAYLOAD/libMoltenVK.teso4m4.dylib" "$PAYLOAD/libMoltenVK.teso4m4.dylib.missing"
if run_tool install --skip-settings --yes >/dev/null 2>&1; then
  print -u2 -- 'Expected the incomplete-payload install to fail.'
  exit 1
fi
mv "$PAYLOAD/libMoltenVK.teso4m4.dylib.missing" "$PAYLOAD/libMoltenVK.teso4m4.dylib"
recovery_output="$(run_tool install --skip-settings --yes)"
[[ "$recovery_output" == *'Restoring the verified baseline before restarting installation.'* ]]
[[ "$recovery_output" == *'Installed to:'* ]]
run_tool remove >/dev/null
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]

install_output="$(run_tool install --apply-settings --settings-file "$SETTINGS_FILE" --yes)"
[[ "$install_output" == *"Installed to: $ESO_APP"* ]]
[[ "$install_output" == *'Release: fixture-1.0.0'* ]]
[[ "$install_output" == *'[1/6] Finding ESO installation'* ]]
[[ "$install_output" == *'━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 100%'* ]]
[[ "$install_output" == *'✓ Installation complete'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" \
  == "$(shasum -a 256 "$PAYLOAD/libBink2Macx64.dylib" | awk '{print $1}')" ]]
[[ -f "$GAME_MAC/.teso4m4-enable" \
  && -f "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  && -f "$GAME_MAC/libMoltenVK.teso4m4.dylib" ]]
grep -q '^mode=startup-pipeline-timing-control$' \
  "$GAME_MAC/.teso4m4-enable"
status_output="$(run_tool status)"
[[ "$status_output" == *'Installed release: fixture-1.0.0'* ]]
reinstall_output="$(run_tool install --skip-settings --yes)"
[[ "$reinstall_output" == *'✓ Patch already installed'* ]]
[[ "$reinstall_output" == *'No files were changed.'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" \
  == "$(shasum -a 256 "$PAYLOAD/libBink2Macx64.dylib" | awk '{print $1}')" ]]
grep -q '^SET FULLSCREEN "2"$' "$SETTINGS_FILE"
grep -q '^SET LANGUAGE.2 "en"$' "$SETTINGS_FILE"
[[ "$(awk '$1 == "SET" {seen[$2]++} END {print seen["FULLSCREEN"]}' "$SETTINGS_FILE")" == 1 ]]

remove_output="$(run_tool remove)"
[[ "$remove_output" == *"Removed from: $ESO_APP"* ]]
[[ "$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')" == "$SETTINGS_ORIGINAL_SHA" ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]
[[ ! -e "$GAME_MAC/.teso4m4-enable" \
  && ! -e "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  && ! -e "$GAME_MAC/libMoltenVK.teso4m4.dylib" ]]

INSTALL_ID="$(print -rn -- "$ESO_APP" | shasum -a 256 | awk '{print $1}')"
mkdir "$STATE_ROOT/$INSTALL_ID/enable-marker"
if run_tool install --apply-settings --settings-file "$SETTINGS_FILE" --yes >/dev/null 2>&1; then
  print -u2 -- 'Expected the post-settings interrupted install to fail.'
  exit 1
fi
rmdir "$STATE_ROOT/$INSTALL_ID/enable-marker"
post_settings_recovery="$(run_tool install --apply-settings --settings-file "$SETTINGS_FILE" --yes)"
[[ "$post_settings_recovery" == *'Restoring the verified baseline before restarting installation.'* ]]
run_tool remove >/dev/null
[[ "$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')" == "$SETTINGS_ORIGINAL_SHA" ]]

run_tool install --apply-settings --settings-file "$SETTINGS_FILE" --yes >/dev/null
print -r -- 'SET USER_CHANGED_AFTER_INSTALL "1"' >> "$SETTINGS_FILE"
CHANGED_SETTINGS_SHA="$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')"
changed_remove_output="$(run_tool remove)"
[[ "$changed_remove_output" == *'Settings changed after installation; they were not overwritten.'* ]]
[[ "$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')" == "$CHANGED_SETTINGS_SHA" ]]

run_tool install --skip-settings --yes >/dev/null
run_tool remove >/dev/null

run_tool install --skip-settings --yes >/dev/null
print -r -- 'compatible fixture update' >> "$GAME_MAC/eso"
cp "$TEST_ROOT/original-bink.dylib" "$GAME_MAC/libBink2Macx64.dylib"
updated_install_output="$(run_tool install --skip-settings --yes)"
[[ "$updated_install_output" == *'Compatible ESO update detected'* ]]
[[ "$updated_install_output" == *'update that restored the original loader'* ]]
UPDATED_ESO_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
grep -q "^eso_sha256=$UPDATED_ESO_SHA$" "$GAME_MAC/.teso4m4-enable"
run_tool remove >/dev/null

run_tool install --skip-settings --yes >/dev/null
print -r -- 'compatible fixture update with bridge retained' >> "$GAME_MAC/eso"
stale_status_output="$(run_tool status)"
[[ "$stale_status_output" == *'stale bridge attestation. Re-run Install.'* ]]
retained_bridge_output="$(run_tool install --skip-settings --yes)"
[[ "$retained_bridge_output" == *'Compatible ESO update detected'* ]]
[[ "$retained_bridge_output" == *'update that left the bridge loader installed'* ]]
RETAINED_UPDATE_SHA="$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')"
grep -q "^eso_sha256=$RETAINED_UPDATE_SHA$" "$GAME_MAC/.teso4m4-enable"
run_tool remove >/dev/null
find "$STATE_ROOT" -name .version -type f -exec grep -q 'fixture-1.0.0' {} \;
print -- 'Release installer transaction: PASS (explicit settings choice, merge, conflict-safe restore, interruption recovery, install/remove/reinstall, compatible-update recovery for restored and retained loaders)'
