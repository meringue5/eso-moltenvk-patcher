#!/bin/zsh
set -euo pipefail

ARCHIVE="${1:?usage: test_release_archive.sh ARCHIVE VERSION}"
VERSION="${2:?usage: test_release_archive.sh ARCHIVE VERSION}"
EXPECTED_ROOT="ESO-MoltenVK-Patcher-$VERSION"
TEST_ROOT="$(mktemp -d /private/tmp/eso-moltenvk-archive-test.XXXXXX)"
trap 'rm -rf "$TEST_ROOT"' EXIT

unzip -tq "$ARCHIVE" >/dev/null
unzip -q "$ARCHIVE" -d "$TEST_ROOT"
STAGE="$TEST_ROOT/$EXPECTED_ROOT"
INTERNAL="$STAGE/.eso-moltenvk-patcher"
[[ -d "$STAGE" && -d "$INTERNAL" ]]

typeset -a visible=(
  Diagnostics.command
  Install.command
  README.txt
  Status.command
  Uninstall.command
)
actual_visible="$(find "$STAGE" -maxdepth 1 -type f -exec basename {} \; | sort)"
expected_visible="$(printf '%s\n' "${visible[@]}" | sort)"
[[ "$actual_visible" == "$expected_visible" ]] || {
  print -u2 -- "Unexpected visible package files:"
  print -u2 -- "$actual_visible"
  exit 1
}

for file in "${visible[@]}"; do
  [[ -f "$STAGE/$file" ]]
done
for command in Diagnostics.command Install.command Status.command Uninstall.command; do
  [[ -x "$STAGE/$command" ]]
done
for file in \
  "$INTERNAL/bin/eso-moltenvk-patcher" \
  "$INTERNAL/status.command" \
  "$INTERNAL/payload/eso-compat-audit" \
  "$INTERNAL/payload/libBink2Macx64.dylib" \
  "$INTERNAL/payload/libMoltenVK.teso4m4.dylib" \
  "$INTERNAL/payload/settings-profile.env" \
  "$INTERNAL/payload/target-profile.env" \
  "$INTERNAL/payload/usersettings-m4-moltenvk-1.4.2-standard.txt" \
  "$INTERNAL/SHA256SUMS.txt"; do
  [[ -f "$file" ]]
done
[[ -x "$INTERNAL/bin/eso-moltenvk-patcher" \
  && -x "$INTERNAL/status.command" \
  && -x "$INTERNAL/payload/eso-compat-audit" ]]
(cd "$INTERNAL" && shasum -a 256 -c SHA256SUMS.txt >/dev/null)
grep -q "^RELEASE_VERSION=$VERSION$" "$INTERNAL/payload/target-profile.env"
grep -q '^SETTINGS_PROFILE_ID=balanced-m4-1920x1200-v1$' \
  "$INTERNAL/payload/settings-profile.env"

metadata_entries="$(unzip -Z1 "$ARCHIVE" | grep -E '(^|/)(__MACOSX|\._|\.DS_Store)(/|$)' || true)"
[[ -z "$metadata_entries" ]]
for text_file in "$STAGE"/*.command "$STAGE/README.txt" \
  "$INTERNAL/bin/eso-moltenvk-patcher" "$INTERNAL/status.command" \
  "$INTERNAL/payload"/*.env "$INTERNAL/payload"/*.txt; do
  if LC_ALL=C grep -q $'\r' "$text_file"; then
    print -u2 -- "CRLF is not allowed: $text_file"
    exit 1
  fi
done
if find "$STAGE" -type f \( -name '*.esopc' -o -name 'UserSettings.txt' \
  -o -name '*.ips' -o -name '.DS_Store' \) -print -quit | grep -q .; then
  print -u2 -- "Private or prohibited evidence leaked into the release archive."
  exit 1
fi

print -- "Release archive: PASS (visible commands, checksums, executability, LF, privacy, and ZIP hygiene)"
