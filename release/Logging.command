#!/bin/zsh
ROOT="${0:A:h}"
if (( $# )); then
  exec "$ROOT/.eso-moltenvk-patcher/bin/eso-moltenvk-patcher" logging "$@"
fi
exec "$ROOT/.eso-moltenvk-patcher/bin/eso-moltenvk-patcher" logging --interactive
