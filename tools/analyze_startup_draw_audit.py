#!/usr/bin/env python3
"""Correlate bounded pre-present pixels with submitted draw provenance."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup draw audit enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 "
    "pixel_samples=20 draw_provenance=enabled"
)
PIXEL_BEGIN = (
    "STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1 "
    "generation_2_samples=1,10,20,30,40,50,60,70,80,90,100,110,120,"
    "130,140,150,160,170,180"
)
PIXEL_READY = (
    "STARTUP_PRESENT_PIXEL_READY: synchronization=queue-wait-idle "
    "samples=20 points_per_sample=5"
)
DRAW_BEGIN = (
    "STARTUP_DRAW_AUDIT_BEGIN: generation_limit=2 "
    "generation_2_present_limit=180 max_distinct_pipelines_per_submit=8"
)
FINISH = (
    "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
    "generation=2 ordinal=180"
)
PIXEL = re.compile(
    r"^STARTUP_PRESENT_PIXEL_SUMMARY: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) image_index=\d+ .* format=44 "
    r"samples=(?P<samples>\d+) exact_magenta=(?P<exact>\d+) "
    r"near_magenta=(?P<near>\d+) black=(?P<black>\d+)$"
)
DRAW = re.compile(
    r"^STARTUP_PRESENT_DRAW_SUMMARY: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) image_index=\d+ wait_count=(?P<wait>\d+) "
    r"matched_signals=(?P<matched>\d+) tracked_commands=(?P<tracked>\d+) "
    r"draw_count=(?P<draws>\d+) indexed_draw_count=(?P<indexed>\d+) "
    r"distinct_pipelines=(?P<pipelines>\d+) pipeline_overflow=(?P<overflow>yes|no) "
    r"draw_signature=(?P<signature>[0-9a-f]{16}) "
    r"first_pipeline=[0-9a-f]{16} last_pipeline=[0-9a-f]{16} .*$"
)
PIPELINE = re.compile(
    r"^STARTUP_PRESENT_DRAW_PIPELINE: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) pipeline_index=\d+ "
    r"signature=(?P<signature>[0-9a-f]{16}) "
    r"vertex_hash=(?P<vertex>[0-9a-f]{16}) "
    r"fragment_hash=(?P<fragment>[0-9a-f]{16}) "
    r"shader_hash_complete=(?P<complete>yes|no) "
    r"pipeline_state=(?P<state>tracked|missing)$"
)
EXPECTED = {(1, 1)} | {(2, value) for value in (1, *range(10, 181, 10))}


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup draw audit run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if lines.count(PIXEL_BEGIN) != 1 or lines.count(PIXEL_READY) != 1:
        reasons.append("the exact pre-present pixel sampler was not armed")
    if lines.count(DRAW_BEGIN) != 1:
        reasons.append("the bounded draw-provenance audit was not armed")
    if lines.count(FINISH) != 1:
        reasons.append("the bounded two-generation window did not finish")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_SKIP:") for line in lines):
        reasons.append("at least one scheduled pixel sample was skipped")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_ERROR:") for line in lines):
        reasons.append("at least one scheduled pixel readback failed")
    if any(line.startswith("LIFECYCLE_ERROR:") for line in lines):
        reasons.append("the lifecycle identity tables overflowed or lost state")

    pixels = {
        (int(match.group("generation")), int(match.group("ordinal"))): match
        for line in lines
        if (match := PIXEL.fullmatch(line)) is not None
    }
    draws = {
        (int(match.group("generation")), int(match.group("ordinal"))): match
        for line in lines
        if (match := DRAW.fullmatch(line)) is not None
    }
    if set(pixels) != EXPECTED or len(pixels) != len(EXPECTED):
        reasons.append("the audit did not retain exactly twenty pixel summaries")
    if set(draws) != EXPECTED or len(draws) != len(EXPECTED):
        reasons.append("the audit did not retain matching draw summaries")
    for match in draws.values():
        wait = int(match.group("wait"))
        matched = int(match.group("matched"))
        tracked = int(match.group("tracked"))
        if wait == 0 or matched != wait or tracked == 0:
            reasons.append("a sampled present lacks exact submit-semaphore provenance")
            break
        if match.group("overflow") == "yes":
            reasons.append("a sampled submit exceeded the pipeline identity cap")
            break

    pipelines_by_frame: dict[tuple[int, int], set[str]] = {}
    incomplete_frames: set[tuple[int, int]] = set()
    for line in lines:
        match = PIPELINE.fullmatch(line)
        if not match:
            continue
        key = (int(match.group("generation")), int(match.group("ordinal")))
        pipelines_by_frame.setdefault(key, set()).add(match.group("signature"))
        if match.group("complete") != "yes" or match.group("state") != "tracked":
            incomplete_frames.add(key)
    for key, match in draws.items():
        if len(pipelines_by_frame.get(key, set())) != int(match.group("pipelines")):
            reasons.append("a draw summary lacks its complete pipeline list")
            break

    magenta = {
        key for key, match in pixels.items() if int(match.group("exact")) > 0
    }
    black = {
        key for key, match in pixels.items()
        if int(match.group("black")) == int(match.group("samples"))
    }
    scene = set(pixels) - magenta - black
    if pink_observed and not magenta:
        reasons.append("visible pink was reported but no exact swapchain magenta was sampled")
    if any(
        int(draws[key].group("draws")) == 0 or
        int(draws[key].group("pipelines")) == 0 or key in incomplete_frames
        for key in magenta if key in draws
    ):
        reasons.append("a magenta frame lacks complete draw/pipeline identity")

    common_magenta: set[str] = set()
    if magenta:
        common_magenta = set.intersection(
            *(pipelines_by_frame.get(key, set()) for key in sorted(magenta))
        )
    black_pipelines = set().union(
        *(pipelines_by_frame.get(key, set()) for key in black)
    ) if black else set()
    candidates_out = sorted(common_magenta - black_pipelines)

    print(f"startup-draw-audit-run: {run_id}")
    print(f"startup-draw-audit-pink-observed: {'yes' if pink_observed else 'no'}")
    print(f"startup-draw-audit-magenta-frames: {sorted(magenta)}")
    print(f"startup-draw-audit-black-frames: {sorted(black)}")
    print(f"startup-draw-audit-scene-frames: {sorted(scene)}")
    print(f"startup-draw-audit-candidate-pipelines: {candidates_out}")
    if reasons:
        return "INCONCLUSIVE", reasons
    if len(candidates_out) == 1:
        return "DRAW-PIPELINE-CANDIDATE-ISOLATED", []
    return "DRAW-PROVENANCE-CAPTURED", []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--pink-observed", required=True, choices=("yes", "no"))
    args = parser.parse_args()
    verdict, reasons = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.pink_observed == "yes",
    )
    print(f"startup-draw-audit-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-draw-audit-reason: {reason}")
    return 0 if verdict not in {"INVALID", "INCONCLUSIVE"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
