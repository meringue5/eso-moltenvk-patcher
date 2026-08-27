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
PRODUCTION_LOG="$TEST_ROOT/bridge.log"

mkdir -p "$PACKAGE/bin" "$PAYLOAD" "$GAME_MAC" "$MOCK_BIN"
mkdir -p "${LEGACY_MVK:h}"
mkdir -p "${SETTINGS_FILE:h}"
cp "$ROOT/release/bin/eso-moltenvk-patcher" "$TOOL"
cp "$ROOT/build/libBink2Macx64.dylib" "$PAYLOAD/"
cp "$ROOT/build/libMoltenVK.teso4m4.dylib" "$PAYLOAD/"
cp "$ROOT/config/usersettings-m4-moltenvk-1.4.2-standard.txt" "$PAYLOAD/"
cp "$ROOT/config/settings-profile.env" "$PAYLOAD/"
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
  write_package_manifest
  PATH="$MOCK_BIN:$PATH" ESO_MOLTENVK_PATCHER_STATE_ROOT="$STATE_ROOT" \
    ESO_MOLTENVK_PATCHER_LOG_PATH="$PRODUCTION_LOG" \
    "$TOOL" "$@" --eso-app "$ESO_APP"
}
write_package_manifest() {
  (cd "$PACKAGE" && shasum -a 256 bin/eso-moltenvk-patcher \
    payload/eso-compat-audit payload/libBink2Macx64.dylib \
    payload/libMoltenVK.teso4m4.dylib payload/settings-profile.env \
    payload/target-profile.env \
    payload/usersettings-m4-moltenvk-1.4.2-standard.txt > SHA256SUMS.txt)
}
INSTALL_ID="$(print -rn -- "$ESO_APP" | shasum -a 256 | awk '{print $1}')"

write_package_manifest
print -r -- '# checksum corruption fixture' >> \
  "$PAYLOAD/usersettings-m4-moltenvk-1.4.2-standard.txt"
checksum_output="$(PATH="$MOCK_BIN:$PATH" \
  ESO_MOLTENVK_PATCHER_STATE_ROOT="$STATE_ROOT" \
  ESO_MOLTENVK_PATCHER_LOG_PATH="$PRODUCTION_LOG" \
  "$TOOL" status --eso-app "$ESO_APP" 2>&1 || true)"
[[ "$checksum_output" == *'package checksum verification failed'* ]]
cp "$ROOT/config/usersettings-m4-moltenvk-1.4.2-standard.txt" "$PAYLOAD/"

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
grep -q '^mode=startup-compositor-neutralize-pacing-release$' \
  "$GAME_MAC/.teso4m4-enable"
status_output="$(run_tool status)"
[[ "$status_output" == *'Installed release: fixture-1.0.0'* ]]
[[ "$status_output" == *'Overall: READY'* ]]
[[ "$status_output" == *'Runtime profile: startup-compositor-neutralize-pacing-release'* ]]
[[ "$status_output" == *'Settings profile: balanced-m4-1920x1200-v1 (applied)'* ]]

cat > "$PRODUCTION_LOG" <<EOF
[run=fixture-run] RUN_START: bridge starting log_level=info
[run=fixture-run] STARTUP_COLOR_AUDIT_BEGIN: generation_limit=2 generation_2_present_limit=180
[run=fixture-run] STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 first_present=71 last_present=150 max_suppressed_draws=96 strategy=ordinal-window fallback=forward
[run=fixture-run] MODE: startup compositor neutralize pacing release enabled live_resources=0 metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 synchronous_queue_submits=0 maximize_concurrent_compilation=0
[run=fixture-run] MOLTENVK: loaded path=$TEST_ROOT/private/game/libMoltenVK.dylib
[run=fixture-run] MOLTENVK_CONFIG: live_resources=0 metal_argument_buffers=0 use_mtlheap=1 synchronous_queue_submits=0 command_pooling=1 prefill=0 maximize_concurrent_compilation=0
[run=fixture-run] INACTIVE_PACING_ACTIVE: inactive_100ms_sleep=bypassed focus_callbacks=unmodified active_byte=observed-only transition_log_limit=16
[run=fixture-run] ACTIVE: redirected 17 Vulkan entry points
[run=fixture-run] ESO SHA-256: $EXPECTED_ESO_SHA
[run=fixture-run] INACTIVE_PACING_STATE: transition=1 active=yes action=forward
[run=fixture-run] STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: action=forward reason=present-deadline generation=2 ordinal=150 suppressed_draws=79
[run=fixture-run] STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit generation=2 ordinal=180
EOF
status_output="$(run_tool status)"
[[ "$status_output" == *'Last launch: PASS'* ]]
[[ "$status_output" == *'Initial activity: active=yes (observed)'* ]]
sed 's/maximize_concurrent_compilation=0/maximize_concurrent_compilation=1/g' \
  "$PRODUCTION_LOG" > "$TEST_ROOT/invalid-production.log"
mv "$TEST_ROOT/invalid-production.log" "$PRODUCTION_LOG"
status_output="$(run_tool status)"
[[ "$status_output" == *'Last launch: REVIEW (1 invariant checks failed)'* ]]
sed 's/maximize_concurrent_compilation=1/maximize_concurrent_compilation=0/g' \
  "$PRODUCTION_LOG" > "$TEST_ROOT/valid-production.log"
mv "$TEST_ROOT/valid-production.log" "$PRODUCTION_LOG"
SUPPORT_ZIP="$TEST_ROOT/support.zip"
diagnostics_output="$(run_tool diagnose --output "$SUPPORT_ZIP")"
[[ "$diagnostics_output" == *'Privacy-filtered support report created'* ]]
unzip -tq "$SUPPORT_ZIP" >/dev/null
support_listing="$(unzip -Z1 "$SUPPORT_ZIP")"
[[ "$support_listing" == *'ESO-MoltenVK-Patcher-Support/report.txt'* ]]
[[ "$support_listing" == *'ESO-MoltenVK-Patcher-Support/checksums.txt'* ]]
[[ "$support_listing" == *'ESO-MoltenVK-Patcher-Support/latest-run.log'* ]]
support_text="$(unzip -p "$SUPPORT_ZIP")"
[[ "$support_text" != *"$TEST_ROOT"* ]]
[[ "$support_text" != *'MOLTENVK: loaded path='* ]]
[[ "$support_text" != *'UserSettings.txt'* ]]
[[ "$support_text" != *'branch_offset='* ]]
[[ "$support_text" != *'descriptor_update_signature='* ]]
[[ "$support_text" == *'Last launch: PASS'* ]]
reinstall_output="$(run_tool install --skip-settings --yes)"
[[ "$reinstall_output" == *'✓ Patch already installed'* ]]
[[ "$reinstall_output" == *'No files were changed.'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" \
  == "$(shasum -a 256 "$PAYLOAD/libBink2Macx64.dylib" | awk '{print $1}')" ]]
# A newer patcher may directly replace an earlier bridge only when the ESO
# executable, marker attestation, recovery record, and original generation all
# still match. Skipping settings during that binary-only upgrade must retain
# the earlier settings restore record.
cp "$PAYLOAD/libBink2Macx64.dylib" "$TEST_ROOT/earlier-bridge.dylib"
install_name_tool -id '@executable_path/libBink2Macx64.earlier-patcher.dylib' \
  "$TEST_ROOT/earlier-bridge.dylib"
cp "$TEST_ROOT/earlier-bridge.dylib" "$GAME_MAC/libBink2Macx64.dylib"
print -r -- 'startup-compositor-neutralize-pacing-release' > \
  "$GAME_MAC/.teso4m4-enable"
earlier_status_output="$(run_tool status)"
[[ "$earlier_status_output" == *'verified earlier patcher installed'* ]]
[[ "$earlier_status_output" == *'Overall: UPGRADE AVAILABLE'* ]]
upgrade_output="$(run_tool install --skip-settings --yes)"
[[ "$upgrade_output" == *'verified earlier patcher on the same ESO and original-loader generation'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" \
  == "$(shasum -a 256 "$PAYLOAD/libBink2Macx64.dylib" | awk '{print $1}')" ]]
grep -q '^settings_choice=apply$' "$STATE_ROOT/$INSTALL_ID/install-state.env"
grep -q '^settings_backup=settings-backup-' "$STATE_ROOT/$INSTALL_ID/install-state.env"
grep -q '^SET FULLSCREEN "2"$' "$SETTINGS_FILE"
grep -q '^SET LANGUAGE.2 "en"$' "$SETTINGS_FILE"
[[ "$(awk '$1 == "SET" {seen[$2]++} END {print seen["FULLSCREEN"]}' "$SETTINGS_FILE")" == 1 ]]

cp "$TEST_ROOT/earlier-bridge.dylib" "$GAME_MAC/libBink2Macx64.dylib"
remove_output="$(run_tool remove)"
[[ "$remove_output" == *"Removed from: $ESO_APP"* ]]
[[ "$remove_output" == *'Matching original Bink loader restored'* ]]
[[ "$(shasum -a 256 "$SETTINGS_FILE" | awk '{print $1}')" == "$SETTINGS_ORIGINAL_SHA" ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]
[[ ! -e "$GAME_MAC/.teso4m4-enable" \
  && ! -e "$GAME_MAC/libBink2Macx64.teso4m4-original.dylib" \
  && ! -e "$GAME_MAC/libMoltenVK.teso4m4.dylib" ]]

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
[[ "$stale_status_output" == *'Run the ESO launcher Repair before Install or Uninstall'* ]]
STALE_BRIDGE_SHA="$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')"
STALE_BACKUP_SHA="$(shasum -a 256 "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" | awk '{print $1}')"
retained_bridge_output="$(run_tool install --skip-settings --yes 2>&1 || true)"
[[ "$retained_bridge_output" == *'Run the ESO launcher Repair to materialize the current original Bink library'* ]]
[[ "$retained_bridge_output" == *'previous backup was not restored'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$STALE_BRIDGE_SHA" ]]
[[ "$(shasum -a 256 "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" | awk '{print $1}')" == "$STALE_BACKUP_SHA" ]]
stale_remove_output="$(run_tool remove 2>&1 || true)"
[[ "$stale_remove_output" == *'Run the ESO launcher Repair before Uninstall'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$STALE_BRIDGE_SHA" ]]

# A launcher repair materializes the current original. Uninstall must preserve
# that active original rather than copying the old backup over it.
cp "$TEST_ROOT/original-bink.dylib" "$GAME_MAC/libBink2Macx64.dylib"
repaired_remove_output="$(run_tool remove)"
[[ "$repaired_remove_output" == *'launcher already replaced the bridge'* ]]
[[ "$repaired_remove_output" == *'Launcher-provided Bink loader preserved'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$EXPECTED_BINK_SHA" ]]
[[ ! -e "$GAME_MAC/.teso4m4-enable" ]]

# An old patcher must neither install nor restore its old backup over a new
# launcher-provided original-loader generation.
run_tool install --skip-settings --yes >/dev/null
print -r -- 'compatible fixture update with new original Bink' >> "$GAME_MAC/eso"
cp "$TEST_ROOT/original-bink.dylib" "$TEST_ROOT/new-original-bink.dylib"
install_name_tool -id '@loader_path/libBink2Macx64.new-generation.dylib' \
  "$TEST_ROOT/new-original-bink.dylib"
cp "$TEST_ROOT/new-original-bink.dylib" "$GAME_MAC/libBink2Macx64.dylib"
NEW_ORIGINAL_BINK_SHA="$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')"
OLD_GENERATION_BACKUP_SHA="$(shasum -a 256 "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" | awk '{print $1}')"
new_original_install_output="$(run_tool install --skip-settings --yes 2>&1 || true)"
[[ "$new_original_install_output" == *'does not support that loader generation'* ]]
[[ "$new_original_install_output" == *'existing backup was not restored'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$NEW_ORIGINAL_BINK_SHA" ]]
[[ "$(shasum -a 256 "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" | awk '{print $1}')" == "$OLD_GENERATION_BACKUP_SHA" ]]
new_original_remove_output="$(run_tool remove)"
[[ "$new_original_remove_output" == *'Launcher-provided Bink loader preserved'* ]]
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$NEW_ORIGINAL_BINK_SHA" ]]

# A newer patcher that explicitly supports the new original generation rotates
# the old recovery pair into history, creates a fresh backup, and uninstalls to
# that same new generation.
cp "$TEST_ROOT/new-original-bink.dylib" "$TEST_ROOT/new-retagged-bink.dylib"
install_name_tool -id '@loader_path/libBink2Macx64.teso4m4-original.dylib' \
  "$TEST_ROOT/new-retagged-bink.dylib"
NEW_RETAGGED_BINK_SHA="$(shasum -a 256 "$TEST_ROOT/new-retagged-bink.dylib" | awk '{print $1}')"
sed -i '' "s/^EXPECTED_ORIGINAL_BINK_SHA256=.*/EXPECTED_ORIGINAL_BINK_SHA256=$NEW_ORIGINAL_BINK_SHA/" \
  "$PAYLOAD/target-profile.env"
sed -i '' "s/^EXPECTED_RETAGGED_ORIGINAL_BINK_SHA256=.*/EXPECTED_RETAGGED_ORIGINAL_BINK_SHA256=$NEW_RETAGGED_BINK_SHA/" \
  "$PAYLOAD/target-profile.env"
# Simulate interruption after the previous generation's backup and state were
# archived but before their current pointers were cleared. Rotation must verify
# and resume this exact archive instead of getting stuck or overwriting it.
RECORDED_GENERATION_ESO_SHA="$(sed -n 's/^executable_sha256=//p' \
  "$STATE_ROOT/$INSTALL_ID/install-state.env" | head -1)"
RECOVERY_ARCHIVE_SUFFIX="${RECORDED_GENERATION_ESO_SHA[1,12]}-${OLD_GENERATION_BACKUP_SHA[1,12]}"
cp "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" \
  "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.before-${RECOVERY_ARCHIVE_SUFFIX}.dylib"
cp "$STATE_ROOT/$INSTALL_ID/install-state.env" \
  "$STATE_ROOT/$INSTALL_ID/install-state.before-${RECOVERY_ARCHIVE_SUFFIX}.env"
generation_install_output="$(run_tool install --skip-settings --yes)"
[[ "$generation_install_output" == *'newly supported original Bink generation is active'* ]]
[[ "$generation_install_output" == *'Preserved the previous original-loader generation'* ]]
[[ "$(shasum -a 256 "$STATE_ROOT/$INSTALL_ID/original-libBink2Macx64.dylib" | awk '{print $1}')" == "$NEW_ORIGINAL_BINK_SHA" ]]
find "$STATE_ROOT/$INSTALL_ID" -name 'original-libBink2Macx64.before-*.dylib' \
  -type f -exec shasum -a 256 {} \; | grep -q "^$OLD_GENERATION_BACKUP_SHA "

# RC-to-final promotion may keep the exact bridge payload. A version change or
# incomplete marker must still refresh the package state and executable
# attestation instead of leaving Status on the earlier package version.
sed -i '' 's/^RELEASE_VERSION=.*/RELEASE_VERSION=fixture-1.0.1/' \
  "$PAYLOAD/target-profile.env"
same_payload_preflight="$(run_tool status)"
[[ "$same_payload_preflight" == *'verified bridge installed with an earlier package state'* ]]
[[ "$same_payload_preflight" == *'Overall: UPGRADE AVAILABLE'* ]]
same_payload_upgrade_output="$(run_tool install --skip-settings --yes)"
[[ "$same_payload_upgrade_output" == *'earlier package state; it will be re-attested in place'* ]]
same_payload_status="$(run_tool status)"
[[ "$same_payload_status" == *'Installed release: fixture-1.0.1'* ]]
[[ "$same_payload_status" == *'Overall: READY'* ]]
grep -q "^eso_sha256=$(shasum -a 256 "$GAME_MAC/eso" | awk '{print $1}')$" \
  "$GAME_MAC/.teso4m4-enable"
run_tool remove >/dev/null
[[ "$(shasum -a 256 "$GAME_MAC/libBink2Macx64.dylib" | awk '{print $1}')" == "$NEW_ORIGINAL_BINK_SHA" ]]
find "$STATE_ROOT" -name .version -type f -exec grep -q 'fixture-1.0.1' {} \;
print -- 'Release installer transaction: PASS (package integrity, status/diagnostics privacy, settings profile, interruption recovery, same-generation and same-payload package promotion, update repair gate, external-original preservation, and original-generation rotation)'
