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
