#!/usr/bin/env python3
"""Classify the bounded startup-compositor placeholder neutralizer."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup compositor neutralize enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 "
    "draw_provenance=enabled input_provenance=enabled "
    "pixel_readback=disabled fallback=forward"
)
BEGIN = (
    "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 first_present=60 "
    "last_present=150 max_suppressed_draws=96 fallback=forward"
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
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup compositor neutralize run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if BEGIN not in lines:
        reasons.append("the exact bounded neutralizer was not armed")
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

    suppress_matches = [
        SUPPRESS.fullmatch(line)
        for line in lines
        if line.startswith("STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS:")
    ]
    if not suppress_matches:
        reasons.append("no compositor draw was suppressed")
    elif any(match is None for match in suppress_matches):
        reasons.append("a compositor suppression record was malformed")
    else:
        suppressions = [match for match in suppress_matches if match]
        draws = [int(match.group("draw")) for match in suppressions]
        ordinals = [int(match.group("ordinal")) for match in suppressions]
        descriptors = {match.group("descriptor") for match in suppressions}
        if draws != list(range(1, len(draws) + 1)):
            reasons.append("suppressed draw ordinals were not contiguous")
        if any(
            match.group("generation") != "2"
            or match.group("pipeline") != TARGET_PIPELINE
            for match in suppressions
        ):
            reasons.append("a suppression escaped the exact generation/pipeline")
        if any(ordinal < 60 or ordinal >= 150 for ordinal in ordinals):
            reasons.append("a suppression escaped the bounded present interval")
        if len(descriptors) != 1:
            reasons.append("the suppressed placeholder descriptor state changed")

    latch_matches = [
        LATCH.fullmatch(line)
        for line in lines
        if line.startswith("STARTUP_COMPOSITOR_NEUTRALIZE_LATCH:")
    ]
    valid_latches = [match for match in latch_matches if match]
    if not latch_matches:
        reasons.append("the neutralizer never latched back to forwarding")
    elif any(match is None for match in latch_matches):
        reasons.append("a compositor latch record was malformed")
    elif len(valid_latches) != 1:
        reasons.append("the neutralizer produced more than one terminal latch")
    else:
        latch = valid_latches[0]
        if (
            latch.group("action") != "forward"
            or latch.group("reason") != "descriptor-transition"
        ):
            reasons.append("the neutralizer ended through a fallback path")
        if (
            latch.group("generation") != "2"
            or latch.group("pipeline") != TARGET_PIPELINE
            or not 60 <= int(latch.group("ordinal")) <= 150
        ):
            reasons.append("the forwarding latch escaped the exact target window")
        if suppress_matches and all(suppress_matches):
            placeholder = suppress_matches[0].group("descriptor")
            if latch.group("descriptor") == placeholder:
                reasons.append("the forwarding latch did not cross a descriptor transition")
            if int(latch.group("draws")) != len(suppress_matches):
                reasons.append("the latch suppressed-draw total was inconsistent")

    print(f"startup-compositor-neutralize-run: {run_id}")
    print(
        "startup-compositor-neutralize-pink-observed: "
        f"{'yes' if pink_observed else 'no'}"
    )
    print(
        "startup-compositor-neutralize-suppressed-draws: "
        f"{len(suppress_matches)}"
    )
    if reasons:
        return "INCONCLUSIVE", reasons
    return (
        "COMPOSITOR-PLACEHOLDER-EXCLUDED"
        if pink_observed
        else "COMPOSITOR-PLACEHOLDER-NEUTRALIZED",
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
    print(f"startup-compositor-neutralize-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-compositor-neutralize-reason: {reason}")
    return 0 if verdict != "INCONCLUSIVE" and verdict != "INVALID" else 1


if __name__ == "__main__":
    raise SystemExit(main())
