#!/bin/zsh

teso4m4_resolve_target_manifest() {
  local root="$1"
  if [[ -n "${TESO4M4_TARGET_MANIFEST:-}" ]]; then
    [[ -f "$TESO4M4_TARGET_MANIFEST" ]] || {
      echo "Target manifest does not exist: $TESO4M4_TARGET_MANIFEST" >&2
      return 1
    }
    echo "${TESO4M4_TARGET_MANIFEST:A}"
    return
  fi

  local pointer="$root/config/current-target.txt"
  [[ -f "$pointer" ]] || {
    echo "Current-target pointer is missing: $pointer" >&2
    return 1
  }
  local name="$(<"$pointer")"
  [[ "$name" == targets-eso-*.json && "$name" != */* ]] || {
    echo "Invalid current-target pointer: $name" >&2
    return 1
  }
  local manifest="$root/config/$name"
  [[ -f "$manifest" ]] || {
    echo "Selected target manifest is missing: $manifest" >&2
    return 1
  }
  echo "$manifest"
}

teso4m4_require_process_absent() {
  local label="$1"
  shift
  local process_status=0
  pgrep "$@" >/dev/null 2>&1 || process_status=$?
  if (( process_status == 0 )); then
    echo "$label is running. Exit it before modifying the ESO bundle." >&2
    return 1
  fi
  if (( process_status != 1 )); then
    echo "Could not verify whether $label is running." >&2
    return 1
  fi
}

teso4m4_acf_value() {
  local manifest="$1"
  local key="$2"
  awk -F'"' -v wanted="$key" '$2 == wanted { print $4; exit }' "$manifest"
}

teso4m4_require_bundle_idle() {
  local eso_app="$1"
  [[ -d "$eso_app" ]] || {
    echo "ESO app bundle is missing: $eso_app" >&2
    return 1
  }

  teso4m4_require_process_absent "ESO" -x eso || return 1
  teso4m4_require_process_absent "The ZeniMax launcher" \
    -f '/ZeniMax Online Studios Launcher' || return 1

  command -v lsof >/dev/null 2>&1 || {
    echo "lsof is required to verify that the ESO bundle is idle." >&2
    return 1
  }
  local open_files=""
  local lsof_status=0
  open_files="$(lsof -Fn +D "$eso_app" 2>/dev/null)" || lsof_status=$?
  if [[ -n "$open_files" ]]; then
    echo "A process has files open inside the ESO app bundle." >&2
    echo "$open_files" >&2
    return 1
  fi
  if (( lsof_status != 0 && lsof_status != 1 )); then
    echo "Could not verify whether the ESO app bundle is idle." >&2
    return 1
  fi

  local steam_status=0
  pgrep -f '/Steam/Contents/MacOS/steam_osx' >/dev/null 2>&1 \
    || steam_status=$?
  if (( steam_status != 0 && steam_status != 1 )); then
    echo "Could not verify whether Steam is running." >&2
    return 1
  fi
  if (( steam_status == 1 )); then
    return 0
  fi

  local steamapps_root="${TESO4M4_STEAMAPPS_ROOT:-${eso_app%%/common/*}}"
  [[ "$steamapps_root" != "$eso_app" ]] || {
    echo "Could not resolve steamapps from the ESO app path." >&2
    return 1
  }
  local app_id="306130"
  local manifest="$steamapps_root/appmanifest_${app_id}.acf"
  [[ -f "$manifest" ]] || {
    echo "Steam is running but the ESO app manifest is missing." >&2
    return 1
  }
  local state_flags="$(teso4m4_acf_value "$manifest" StateFlags)"
  local bytes_to_download="$(teso4m4_acf_value "$manifest" BytesToDownload)"
  local bytes_downloaded="$(teso4m4_acf_value "$manifest" BytesDownloaded)"
  [[ "$state_flags" == "4" \
    && -n "$bytes_to_download" \
    && "$bytes_to_download" == "$bytes_downloaded" ]] || {
    echo "Steam reports ESO as updating, incomplete, or not fully installed." >&2
    return 1
  }

  local download_dir="$steamapps_root/downloading/$app_id"
  if [[ -d "$download_dir" \
    && -n "$(find "$download_dir" -mindepth 1 -print -quit 2>/dev/null)" ]]; then
    echo "Steam has staged ESO download content; refusing bundle modification." >&2
    return 1
  fi

  echo "ESO bundle idle: Steam is open but has no ESO file or update activity."
}
