#!/bin/zsh
set -euo pipefail

ROOT="${0:A:h:h}"
TRIALS="${1:-10}"
PROBE="$ROOT/build/probe_vulkan"
RUNTIME="$ROOT/build/libMoltenVK.teso4m4.dylib"

[[ "$TRIALS" == <-> && "$TRIALS" -gt 0 ]] || {
  echo "usage: $0 [positive-trial-count]"
  exit 2
}
for file in "$PROBE" "$RUNTIME"; do
  [[ -f "$file" ]] || {
    echo "Missing build artifact: $file"
    echo "Run scripts/build.sh first."
    exit 1
  }
done

failures=0
for trial in {1..$TRIALS}; do
  set +e
  output="$(
    MVK_CONFIG_LOG_LEVEL=0 \
    MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=0 \
    MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS=0 \
    MVK_CONFIG_USE_MTLHEAP=1 \
    MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0 \
    MVK_CONFIG_USE_COMMAND_POOLING=1 \
    MVK_CONFIG_PREFILL_METAL_COMMAND_BUFFERS=0 \
    MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION=1 \
    TESO4M4_PROBE_HDR_FILTER=1 \
    TESO4M4_PROBE_READINESS=1 \
      "$PROBE" "$RUNTIME" 2>&1
  )"
  result=$?
  set -e
  readiness="${${(M)${(f)output}:#*RUNTIME_READINESS: compiler_canary=pass*}:-}"
  if [[ "$result" -eq 0 && -n "$readiness" ]]; then
    duration="${${readiness##*duration_ns=}%%[^0-9]*}"
    echo "trial=$trial result=PASS duration_ns=$duration"
  else
    (( failures += 1 ))
    echo "trial=$trial result=FAIL exit=$result"
    print -r -- "$output" | grep -E \
      'RUNTIME_READINESS|CREATE_DEVICE_ERROR|vkCreateDevice' || true
  fi
done

echo "Runtime readiness probe: trials=$TRIALS failures=$failures"
[[ "$failures" -eq 0 ]]
