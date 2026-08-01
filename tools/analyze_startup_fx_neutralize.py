#!/usr/bin/env python3
"""Classify the bounded FX-material sentinel intervention."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup FX neutralize enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180"
)
BEGIN = (
    "STARTUP_FX_SENTINEL_BEGIN: initializer_offset=0x35fcd42 "
    "window=generation-2-present-180 vectors=0x10,0x20,0x30 "
    "replacement=black-preserve-alpha"
)
FINISH = (
    "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
    "generation=2 ordinal=180"
)
EVENT = re.compile(
    r"^STARTUP_FX_SENTINEL: call=(?P<call>\d+) match=(?P<match>yes|no) "
    r"match_count=(?P<count>\d+) caller="
    r"(?P<caller>fx-material|fx-material-transparent|other) "
    r"return_offset=0x(?P<return>[0-9a-f]+)$"
)


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup FX neutralize run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if BEGIN not in lines:
        reasons.append("the exact FX sentinel patch was not installed")
    if FINISH not in lines:
        reasons.append("the bounded two-generation window did not finish")
    events = [EVENT.fullmatch(line) for line in lines if line.startswith(
        "STARTUP_FX_SENTINEL:"
    )]
    if not events:
        reasons.append("the FX initializer was not observed inside the startup window")
    elif any(match is None for match in events):
        reasons.append("an FX sentinel event was malformed")
    else:
        parsed = [match for match in events if match is not None]
        calls = [int(match.group("call")) for match in parsed]
        if calls != list(range(1, len(calls) + 1)):
            reasons.append("FX sentinel call ordinals were not contiguous")
        if any(match.group("match") != "yes" for match in parsed):
            reasons.append("at least one FX initializer output did not match the sentinel")
        if any(match.group("caller") == "other" for match in parsed):
            reasons.append("an FX initializer call came from an unprofiled caller")
        expected_callers = {
            ("fx-material", "1ba0dc"),
            ("fx-material-transparent", "1bb46d"),
        }
        if any(
            (match.group("caller"), match.group("return"))
            not in expected_callers
            for match in parsed
        ):
            reasons.append("an FX initializer caller identity was not exact")
    print(f"startup-fx-run: {run_id}")
    print(f"startup-fx-pink-observed: {'yes' if pink_observed else 'no'}")
    print(f"startup-fx-events: {len(events)}")
    if reasons:
        return "INCONCLUSIVE", reasons
    return (
        "FX-SENTINEL-EXCLUDED" if pink_observed else "FX-SENTINEL-CAUSAL",
        [],
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument(
        "--pink-observed", required=True, choices=("yes", "no")
    )
    args = parser.parse_args()
    verdict, reasons = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.pink_observed == "yes",
    )
    print(f"startup-fx-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-fx-reason: {reason}")
    return 0 if verdict in {"FX-SENTINEL-EXCLUDED", "FX-SENTINEL-CAUSAL"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
