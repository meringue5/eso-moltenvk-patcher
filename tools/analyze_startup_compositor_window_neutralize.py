#!/usr/bin/env python3
"""Classify the fixed-window startup compositor neutralizer."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


LEGACY_MODE = (
    "MODE: startup compositor neutralize enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 "
    "draw_provenance=enabled input_provenance=enabled "
    "pixel_readback=disabled fallback=forward"
)
TIMING_MODE = (
    "MODE: startup compositor neutralize enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=0 "
    "generation_limit=2 generation_2_present_limit=180 "
    "draw_provenance=enabled input_provenance=enabled "
    "pipeline_timing=bounded readiness_canary=disabled "
    "pixel_readback=disabled fallback=forward"
)
PACING_BYPASS_MODE = (
    "MODE: startup compositor neutralize pacing bypass enabled "
    "live_resources=0 metal_argument_buffers=0 use_mtlheap=1 "
    "command_pooling=1 synchronous_queue_submits=0 "
    "maximize_concurrent_compilation=0 generation_limit=2 "
    "generation_2_present_limit=180 draw_provenance=enabled "
    "input_provenance=enabled pipeline_timing=bounded "
    "readiness_canary=disabled pixel_readback=disabled "
    "fallback=forward inactive_100ms_sleep=bypassed"
)
RELEASE_MODE = (
    "MODE: startup compositor neutralize pacing release enabled "
    "live_resources=0 metal_argument_buffers=0 use_mtlheap=1 "
    "command_pooling=1 synchronous_queue_submits=0 "
    "maximize_concurrent_compilation=0 generation_limit=2 "
    "generation_2_present_limit=180 draw_provenance=bounded "
    "input_provenance=bounded pipeline_timing=disabled "
    "readiness_canary=disabled pixel_readback=disabled "
    "post_window_bookkeeping=disabled fallback=forward "
    "inactive_100ms_sleep=bypassed"
)
MODES = (LEGACY_MODE, TIMING_MODE, PACING_BYPASS_MODE, RELEASE_MODE)
PACING_READY = (
    "INACTIVE_PACING_READY: branch_offset=0x30f78 "
    "active_flag_offset=0x4a0c93c original_delay_us=100000 "
    "replacement=observe-and-return"
)
PACING_ACTIVE = (
    "INACTIVE_PACING_ACTIVE: inactive_100ms_sleep=bypassed "
    "focus_callbacks=unmodified active_byte=observed-only "
    "transition_log_limit=16"
)
BEGIN = (
    "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 first_present=71 "
    "last_present=150 max_suppressed_draws=96 strategy=ordinal-window "
    "fallback=forward"
)
FINISH = (
    "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
    "generation=2 ordinal=180"
)
TARGET_PIPELINE = "c43e4410d3b33fe7"
SUPPRESS = re.compile(
    r"^STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS: generation=(?P<generation>\d+) "
    r"ordinal=(?P<ordinal>\d+) pipeline=(?P<pipeline>[0-9a-f]{16}) "
    r"descriptor_update_signature=(?P<descriptor>[0-9a-f]{16}) "
    r"draw=(?P<draw>\d+)$"
)
LATCH = re.compile(
    r"^STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: action=(?P<action>forward|abort) "
    r"reason=(?P<reason>[a-z-]+) generation=(?P<generation>\d+) "
    r"ordinal=(?P<ordinal>\d+) pipeline=(?P<pipeline>[0-9a-f]{16}) "
    r"descriptor_update_signature=(?P<descriptor>[0-9a-f]{16}) "
    r"suppressed_draws=(?P<draws>\d+)$"
)


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if any(mode in lines for mode in MODES) and BEGIN in lines
    ]
    if not candidates:
        return "INVALID", ["no fixed-window compositor neutralizer run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if PACING_BYPASS_MODE in lines or RELEASE_MODE in lines:
        if lines.count(PACING_READY) != 1 or lines.count(PACING_ACTIVE) != 1:
            reasons.append("the exact inactive pacing bypass was not active")
    if RELEASE_MODE in lines and any(
        line.startswith("STARTUP_PIPELINE_") for line in lines
    ):
        reasons.append("release mode unexpectedly enabled pipeline timing")
    if FINISH not in lines:
        reasons.append("the bounded two-generation window did not finish")
    if any(
        line.startswith("STARTUP_PRESENT_PIXEL_READY:")
        or line.startswith("STARTUP_COMPOSITOR_IMAGE_READY:")
        for line in lines
    ):
        reasons.append("pixel readback was unexpectedly enabled")
    if any(
        line.startswith("LIFECYCLE_ERROR:")
        or line.startswith("STARTUP_COLOR_DETAIL_LIMIT:")
        for line in lines
    ):
        reasons.append("lifecycle tracking reported an error or truncation")

    raw_suppressions = [
        SUPPRESS.fullmatch(line)
        for line in lines
        if line.startswith("STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS:")
    ]
    suppressions = [match for match in raw_suppressions if match]
    if not raw_suppressions:
        reasons.append("no compositor draw was suppressed")
    elif len(suppressions) != len(raw_suppressions):
        reasons.append("a compositor suppression record was malformed")
    else:
        draws = [int(match.group("draw")) for match in suppressions]
        ordinals = [int(match.group("ordinal")) for match in suppressions]
        if draws != list(range(1, len(draws) + 1)):
            reasons.append("suppressed draw counters were not contiguous")
        if any(
            match.group("generation") != "2"
            or match.group("pipeline") != TARGET_PIPELINE
            for match in suppressions
        ):
            reasons.append("a suppression escaped the exact generation/pipeline")
        if any(ordinal < 71 or ordinal >= 150 for ordinal in ordinals):
            reasons.append("a suppression escaped the fixed present window")
        if min(ordinals) > 80:
            reasons.append("suppression began after the proven magenta interval")
        if max(ordinals) < 140:
            reasons.append("suppression ended before the proven magenta interval")

    raw_latches = [
        LATCH.fullmatch(line)
        for line in lines
        if line.startswith("STARTUP_COMPOSITOR_NEUTRALIZE_LATCH:")
    ]
    latches = [match for match in raw_latches if match]
    if not raw_latches:
        reasons.append("the neutralizer never latched back to forwarding")
    elif len(latches) != len(raw_latches):
        reasons.append("a compositor latch record was malformed")
    elif len(latches) != 1:
        reasons.append("the neutralizer produced more than one terminal latch")
    else:
        latch = latches[0]
        if (
            latch.group("action") != "forward"
            or latch.group("reason") != "present-deadline"
            or latch.group("generation") != "2"
            or int(latch.group("ordinal")) != 150
            or latch.group("pipeline") != TARGET_PIPELINE
        ):
            reasons.append("the neutralizer did not forward at the exact deadline")
        if suppressions and int(latch.group("draws")) != len(suppressions):
            reasons.append("the latch suppressed-draw total was inconsistent")

    print(f"startup-compositor-window-neutralize-run: {run_id}")
    print(
        "startup-compositor-window-neutralize-pink-observed: "
        f"{'yes' if pink_observed else 'no'}"
    )
    print(
        "startup-compositor-window-neutralize-suppressed-draws: "
        f"{len(raw_suppressions)}"
    )
    if reasons:
        return "INCONCLUSIVE", reasons
    return (
        "WINDOW-NEUTRALIZATION-FAILED"
        if pink_observed
        else "WINDOW-NEUTRALIZED",
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
    print(f"startup-compositor-window-neutralize-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-compositor-window-neutralize-reason: {reason}")
    return 0 if verdict not in ("INCONCLUSIVE", "INVALID") else 1


if __name__ == "__main__":
    raise SystemExit(main())
