#!/usr/bin/env python3
"""Classify bounded startup draw inputs across black, magenta, and scene frames."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from analyze_startup_draw_audit import DRAW, EXPECTED, PIPELINE, PIXEL
from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup input audit enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 "
    "pixel_samples=20 draw_provenance=enabled input_provenance=enabled"
)
PIXEL_BEGIN = (
    "STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1 "
    "generation_2_samples=1,10,20,30,40,50,60,70,80,90,100,110,120,"
    "130,140,150,160,170,180"
)
DRAW_BEGIN = (
    "STARTUP_DRAW_AUDIT_BEGIN: generation_limit=2 "
    "generation_2_present_limit=180 max_distinct_pipelines_per_submit=8"
)
INPUT_BEGIN = (
    "STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2 "
    "generation_2_present_limit=180 max_descriptor_set_layouts=2048 "
    "max_pipeline_layouts=2048 max_descriptor_sets=131072 max_bound_sets=16"
)
FINISH = (
    "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
    "generation=2 ordinal=180"
)
INPUT = re.compile(
    r"^STARTUP_PRESENT_DRAW_INPUT: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) bound_sets=(?P<sets>\d+) "
    r"descriptor_layout_signature=(?P<layout>[0-9a-f]{16}) "
    r"descriptor_handle_signature=(?P<handles>[0-9a-f]{16}) "
    r"descriptor_update_signature=(?P<updates>[0-9a-f]{16}) "
    r"push_signature=(?P<push>[0-9a-f]{16}) "
    r"push_bytes=(?P<push_bytes>\d+) input_complete=(?P<complete>yes|no)$"
)
INPUT_PIPELINE = re.compile(
    r"^STARTUP_PRESENT_INPUT_PIPELINE: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) pipeline_index=\d+ "
    r"pipeline_signature=(?P<pipeline>[0-9a-f]{16}) "
    r"layout_signature=(?P<layout>[0-9a-f]{16}) "
    r"set_layouts=(?P<sets>\d+) descriptors=(?P<descriptors>\d+) "
    r"images=(?P<images>\d+) buffers=(?P<buffers>\d+) "
    r"samplers=(?P<samplers>\d+) "
    r"input_attachments=(?P<input_attachments>\d+) "
    r"push_bytes=(?P<push_bytes>\d+) "
    r"layout_complete=(?P<complete>yes|no)$"
)


def values(matches: dict[tuple[int, int], re.Match[str]], keys: set[tuple[int, int]], field: str) -> list[str]:
    return sorted({matches[key].group(field) for key in keys})


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup input audit run was found"]
    _, _, run_id, lines = max(
        candidates,
        key=lambda item: (
            item[0] if item[0] is not None else float("-inf"), item[1]
        ),
    )
    reasons: list[str] = []
    if lines.count(PIXEL_BEGIN) != 1 or lines.count(DRAW_BEGIN) != 1:
        reasons.append("the inherited pixel/draw audit was not armed exactly once")
    if lines.count(INPUT_BEGIN) != 1:
        reasons.append("the bounded input audit was not armed exactly once")
    if lines.count(FINISH) != 1:
        reasons.append("the bounded two-generation window did not finish")
    if any(line.startswith("LIFECYCLE_ERROR:") for line in lines):
        reasons.append("an input identity table overflowed or lost state")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_SKIP:") for line in lines):
        reasons.append("at least one scheduled pixel sample was skipped")

    pixels = {
        (int(match.group("generation")), int(match.group("ordinal"))): match
        for line in lines if (match := PIXEL.fullmatch(line)) is not None
    }
    draws = {
        (int(match.group("generation")), int(match.group("ordinal"))): match
        for line in lines if (match := DRAW.fullmatch(line)) is not None
    }
    inputs = {
        (int(match.group("generation")), int(match.group("ordinal"))): match
        for line in lines if (match := INPUT.fullmatch(line)) is not None
    }
    if set(pixels) != EXPECTED or set(draws) != EXPECTED or set(inputs) != EXPECTED:
        reasons.append("the audit did not retain exactly twenty aligned samples")

    magenta = {
        key for key, match in pixels.items() if int(match.group("exact")) > 0
    }
    black = {
        key for key, match in pixels.items()
        if int(match.group("black")) == int(match.group("samples"))
    }
    scene = set(pixels) - magenta - black
    if pink_observed and not magenta:
        reasons.append("visible pink was reported but no exact magenta was sampled")
    if not scene:
        reasons.append("no later normal-scene sample exists for input comparison")

    pipelines_by_frame: dict[tuple[int, int], set[str]] = {}
    for line in lines:
        match = PIPELINE.fullmatch(line)
        if match:
            key = (int(match.group("generation")), int(match.group("ordinal")))
            if match.group("complete") != "yes" or match.group("state") != "tracked":
                reasons.append("a sampled graphics pipeline identity is incomplete")
            pipelines_by_frame.setdefault(key, set()).add(match.group("signature"))
    common_magenta = set.intersection(
        *(pipelines_by_frame.get(key, set()) for key in sorted(magenta))
    ) if magenta else set()
    black_pipelines = set().union(
        *(pipelines_by_frame.get(key, set()) for key in black)
    ) if black else set()
    candidate_pipelines = sorted(common_magenta - black_pipelines)
    if len(candidate_pipelines) != 1:
        reasons.append("the magenta interval does not isolate exactly one pipeline")

    input_pipelines: dict[tuple[int, int], list[re.Match[str]]] = {}
    for line in lines:
        match = INPUT_PIPELINE.fullmatch(line)
        if match:
            key = (int(match.group("generation")), int(match.group("ordinal")))
            input_pipelines.setdefault(key, []).append(match)

    compared = magenta | scene
    target_pipeline = candidate_pipelines[0] if len(candidate_pipelines) == 1 else None
    target_layouts: list[re.Match[str]] = []
    for key in compared:
        matches = [
            match for match in input_pipelines.get(key, [])
            if match.group("pipeline") == target_pipeline
        ]
        if len(matches) != 1:
            reasons.append("a compared frame lacks one exact input-layout identity")
            continue
        target_layouts.append(matches[0])
        if matches[0].group("complete") != "yes":
            reasons.append("the target pipeline layout identity is incomplete")
        if key not in inputs or inputs[key].group("complete") != "yes":
            reasons.append("a compared draw lacks complete bound-input provenance")

    layout_tuples = {
        tuple(match.group(field) for field in (
            "layout", "sets", "descriptors", "images", "buffers",
            "samplers", "input_attachments", "push_bytes"
        ))
        for match in target_layouts
    }
    if len(layout_tuples) != 1:
        reasons.append("the target pipeline layout changes across compared frames")

    print(f"startup-input-audit-run: {run_id}")
    print(f"startup-input-audit-pink-observed: {'yes' if pink_observed else 'no'}")
    print(f"startup-input-audit-magenta-frames: {sorted(magenta)}")
    print(f"startup-input-audit-black-frames: {sorted(black)}")
    print(f"startup-input-audit-scene-frames: {sorted(scene)}")
    print(f"startup-input-audit-candidate-pipelines: {candidate_pipelines}")

    if reasons:
        return "INCONCLUSIVE", list(dict.fromkeys(reasons))

    assert target_layouts and layout_tuples
    layout = target_layouts[0]
    for field in (
        "layout", "sets", "descriptors", "images", "buffers", "samplers",
        "input_attachments", "push_bytes"
    ):
        print(f"startup-input-audit-layout-{field}: {layout.group(field)}")

    comparisons = {
        field: (
            values(inputs, magenta, field),
            values(inputs, scene, field),
        )
        for field in ("sets", "layout", "handles", "updates", "push", "push_bytes")
    }
    for field, (magenta_values, scene_values) in comparisons.items():
        print(f"startup-input-audit-magenta-{field}: {magenta_values}")
        print(f"startup-input-audit-scene-{field}: {scene_values}")

    stable_handles = comparisons["handles"][0] == comparisons["handles"][1]
    stable_updates = comparisons["updates"][0] == comparisons["updates"][1]
    stable_push = comparisons["push"][0] == comparisons["push"][1]
    images = int(layout.group("images"))
    buffers = int(layout.group("buffers"))

    if stable_handles and stable_updates and stable_push and (images or buffers):
        return "BOUND-RESOURCE-CONTENT-CANDIDATE", []
    if not stable_updates or not stable_handles:
        return "DESCRIPTOR-STATE-CHANGE-CANDIDATE", []
    if not stable_push:
        return "PUSH-CONSTANT-CHANGE-CANDIDATE", []
    return "INPUT-PROVENANCE-CAPTURED", []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--pink-observed", required=True, choices=("yes", "no"))
    args = parser.parse_args()
    verdict, reasons = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.pink_observed == "yes",
    )
    print(f"startup-input-audit-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-input-audit-reason: {reason}")
    return 0 if verdict not in {"INVALID", "INCONCLUSIVE"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
