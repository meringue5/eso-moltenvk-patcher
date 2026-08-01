#!/usr/bin/env python3
"""Classify the bounded pre-present swapchain pixel audit."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup present pixel audit enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 pixel_samples=8"
)
BEGIN = (
    "STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1 "
    "generation_2_samples=1,30,60,90,120,150,180"
)
READY = (
    "STARTUP_PRESENT_PIXEL_READY: synchronization=queue-wait-idle "
    "samples=8 points_per_sample=5"
)
FINISH = (
    "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
    "generation=2 ordinal=180"
)
SUMMARY = re.compile(
    r"^STARTUP_PRESENT_PIXEL_SUMMARY: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) image_index=\d+ "
    r"requested_extent=\d+x\d+ texture_extent=\d+x\d+ format=44 "
    r"samples=(?P<samples>\d+) exact_magenta=(?P<exact>\d+) "
    r"near_magenta=(?P<near>\d+) black=(?P<black>\d+)$"
)
EXPECTED = {(1, 1)} | {(2, value) for value in (1, 30, 60, 90, 120, 150, 180)}


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup present pixel audit run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if lines.count(BEGIN) != 1:
        reasons.append("the exact eight-frame sampling schedule was not armed")
    if lines.count(READY) != 1:
        reasons.append("the Metal texture sampler was not ready exactly once")
    if lines.count(FINISH) != 1:
        reasons.append("the bounded two-generation window did not finish")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_ERROR:") for line in lines):
        reasons.append("at least one Metal pixel readback failed")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_SKIP:") for line in lines):
        reasons.append("at least one scheduled frame was not safely sampled")

    summaries = [
        match
        for line in lines
        if (match := SUMMARY.fullmatch(line)) is not None
    ]
    keys = {
        (int(match.group("generation")), int(match.group("ordinal")))
        for match in summaries
    }
    if keys != EXPECTED or len(summaries) != len(EXPECTED):
        reasons.append("the audit did not retain exactly all eight scheduled frames")
    if any(int(match.group("samples")) != 5 for match in summaries):
        reasons.append("a scheduled frame did not retain all five pixel points")
    if any(
        int(match.group("exact")) > int(match.group("near"))
        or int(match.group("near")) > int(match.group("samples"))
        or int(match.group("black")) > int(match.group("samples"))
        for match in summaries
    ):
        reasons.append("a pixel summary contained impossible counts")

    near_frames = [
        (int(match.group("generation")), int(match.group("ordinal")))
        for match in summaries
        if int(match.group("near")) > 0
    ]
    exact_frames = [
        (int(match.group("generation")), int(match.group("ordinal")))
        for match in summaries
        if int(match.group("exact")) > 0
    ]
    print(f"startup-present-pixel-run: {run_id}")
    print(f"startup-present-pixel-pink-observed: {'yes' if pink_observed else 'no'}")
    print(f"startup-present-pixel-summaries: {len(summaries)}")
    print(f"startup-present-pixel-near-magenta-frames: {near_frames}")
    print(f"startup-present-pixel-exact-magenta-frames: {exact_frames}")
    if reasons:
        return "INCONCLUSIVE", reasons
    if pink_observed:
        return (
            "SWAPCHAIN-MAGENTA-CONFIRMED"
            if near_frames
            else "POST-SWAPCHAIN-MAGENTA"
        ), []
    return (
        "SWAPCHAIN-MAGENTA-NOT-DISPLAYED"
        if near_frames
        else "NO-PINK-CONTROL"
    ), []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--pink-observed", required=True, choices=("yes", "no"))
    args = parser.parse_args()
    verdict, reasons = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.pink_observed == "yes",
    )
    print(f"startup-present-pixel-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-present-pixel-reason: {reason}")
    return 0 if verdict not in {"INVALID", "INCONCLUSIVE"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
