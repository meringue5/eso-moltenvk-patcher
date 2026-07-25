#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
LOG_DIR="${ESO_LAUNCHER_LOG_DIR:-$HOME/Library/Application Support/ZeniMax Online Studios/public/Bethesda.net_Launcher_Mac}"

exec python3 "$ROOT/tools/launcher_state.py" check --log-dir "$LOG_DIR"
