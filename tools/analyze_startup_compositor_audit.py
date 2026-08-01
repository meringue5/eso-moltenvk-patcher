#!/usr/bin/env python3
"""Classify image versus buffer changes at ESO's startup compositor."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from analyze_startup_draw_audit import DRAW, EXPECTED, PIPELINE, PIXEL
from analyze_startup_input_audit import DRAW_BEGIN, FINISH, INPUT, INPUT_BEGIN, PIXEL_BEGIN
from check_startup_log import parse_runs, run_epoch


MODE = (
    "MODE: startup compositor audit enabled live_resources=0 "
    "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
    "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
    "generation_limit=2 generation_2_present_limit=180 "
    "pixel_samples=20 draw_provenance=enabled input_provenance=enabled "
    "descriptor_classes=enabled"
)
TARGET_PIPELINE = "c43e4410d3b33fe7"
TARGET_VERTEX = "c8307556011c995e"
TARGET_FRAGMENT = "6907bd3576e3a930"
TARGET_LAYOUTS = {
    0: ("e3c2499a89df1706", 0, 3),
    1: ("d0edad262f8c4230", 2, 3),
}
DESCRIPTOR_CLASS = re.compile(
    r"^STARTUP_PRESENT_DESCRIPTOR_CLASS: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) slot=(?P<slot>\d+) "
    r"layout_signature=(?P<layout>[0-9a-f]{16}) "
    r"expected_images=(?P<images>\d+) expected_buffers=(?P<buffers>\d+) "
    r"image_update_signature=(?P<image_signature>[0-9a-f]{16}) "
    r"image_update_writes=(?P<image_writes>\d+) "
    r"image_update_call=(?P<image_call>\d+) "
    r"buffer_update_signature=(?P<buffer_signature>[0-9a-f]{16}) "
    r"buffer_update_writes=(?P<buffer_writes>\d+) "
    r"buffer_update_call=(?P<buffer_call>\d+) "
    r"class_complete=(?P<complete>yes|no)$"
)
COMPOSITOR_IMAGE = re.compile(
    r"^STARTUP_PRESENT_COMPOSITOR_IMAGE: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) set_slot=(?P<slot>\d+) "
    r"image_ordinal=(?P<image_ordinal>\d+) binding=(?P<binding>\d+) "
    r"array_element=(?P<array>\d+) signature=(?P<signature>[0-9a-f]{16}) "
    r"update_call=\d+ view=\S+ image=\S+ format=-?\d+ view_type=\d+ "
    r"mip=\d+ layer=\d+ layout=-?\d+$"
)
COMPOSITOR_SUMMARY = re.compile(
    r"^STARTUP_COMPOSITOR_IMAGE_SUMMARY: generation=(?P<generation>[12]) "
    r"ordinal=(?P<ordinal>\d+) set_slot=(?P<slot>\d+) "
    r"binding=(?P<binding>\d+) array_element=(?P<array>\d+) "
    r"image_ordinal=(?P<image_ordinal>\d+) vk_format=-?\d+ "
    r"metal_format=\d+ mip=\d+ layer=\d+ extent=\d+x\d+ "
    r"samples=(?P<samples>\d+) exact_magenta=(?P<exact>\d+) "
    r"near_magenta=(?P<near>\d+) black=(?P<black>\d+)$"
)


def _values(
    classes: dict[tuple[int, int, int], re.Match[str]],
    frames: set[tuple[int, int]],
    slot: int,
    field: str,
) -> list[str]:
    return sorted({classes[(*frame, slot)].group(field) for frame in frames})


def analyze(text: str, pink_observed: bool) -> tuple[str, list[str]]:
    candidates = [
        (run_epoch(run_id), order, run_id, lines)
        for order, (run_id, lines) in enumerate(parse_runs(text).items())
        if MODE in lines
    ]
    if not candidates:
        return "INVALID", ["no startup compositor audit run was found"]
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
    if lines.count(
        "STARTUP_COMPOSITOR_AUDIT_BEGIN: image_bindings_per_set=2 "
        "sampled_subresources=base-mip-base-layer"
    ) != 1:
        reasons.append("the compositor input audit was not armed exactly once")
    if lines.count(
        "STARTUP_COMPOSITOR_IMAGE_READY: synchronization=queue-wait-idle "
        "points_per_image=5 formats=rgba8,bgra8,rgba16f"
    ) != 1:
        reasons.append("the compositor image sampler was not ready exactly")
    if lines.count(FINISH) != 1:
        reasons.append("the bounded two-generation window did not finish")
    if any(line.startswith("LIFECYCLE_ERROR:") for line in lines):
        reasons.append("an input identity table overflowed or lost state")
    if any(line.startswith("STARTUP_PRESENT_PIXEL_SKIP:") for line in lines):
        reasons.append("at least one scheduled pixel sample was skipped")
    if any(
        line.startswith("STARTUP_PRESENT_COMPOSITOR_IMAGE_SKIP:") or
        line.startswith("STARTUP_COMPOSITOR_IMAGE_ERROR:")
        for line in lines
    ):
        reasons.append("at least one compositor input sample was skipped")

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

    magenta = {key for key, match in pixels.items() if int(match.group("exact")) > 0}
    black = {
        key for key, match in pixels.items()
        if int(match.group("black")) == int(match.group("samples"))
    }
    scene = set(pixels) - magenta - black
    compared = magenta | scene
    if pink_observed and not magenta:
        reasons.append("visible pink was reported but no exact magenta was sampled")
    if not scene:
        reasons.append("no later normal-scene sample exists for comparison")

    target_frames: set[tuple[int, int]] = set()
    for line in lines:
        match = PIPELINE.fullmatch(line)
        if not match or match.group("signature") != TARGET_PIPELINE:
            continue
        key = (int(match.group("generation")), int(match.group("ordinal")))
        if (
            match.group("vertex") != TARGET_VERTEX
            or match.group("fragment") != TARGET_FRAGMENT
            or match.group("complete") != "yes"
            or match.group("state") != "tracked"
        ):
            reasons.append("the startup compositor pipeline identity is incomplete")
        target_frames.add(key)
    if not compared or not compared.issubset(target_frames):
        reasons.append("a compared frame lacks the exact startup compositor pipeline")
    for key in compared:
        if key not in inputs or inputs[key].group("complete") != "yes":
            reasons.append("a compared draw lacks complete bound-input provenance")
        elif inputs[key].group("sets") != "2":
            reasons.append("the startup compositor did not bind exactly two sets")

    classes: dict[tuple[int, int, int], re.Match[str]] = {}
    for line in lines:
        match = DESCRIPTOR_CLASS.fullmatch(line)
        if match:
            key = (
                int(match.group("generation")),
                int(match.group("ordinal")),
                int(match.group("slot")),
            )
            if key in classes:
                reasons.append("a descriptor-class sample was duplicated")
            classes[key] = match
    for frame in compared:
        for slot, (layout, images, buffers) in TARGET_LAYOUTS.items():
            match = classes.get((*frame, slot))
            if not match:
                reasons.append("a compared frame lacks a descriptor-class sample")
                continue
            if (
                match.group("layout") != layout
                or int(match.group("images")) != images
                or int(match.group("buffers")) != buffers
                or match.group("complete") != "yes"
            ):
                reasons.append("a descriptor-class sample does not match the compositor layout")
            if images and int(match.group("image_writes")) == 0:
                reasons.append("the compositor image update batch is missing")
            if buffers and int(match.group("buffer_writes")) == 0:
                reasons.append("a compositor buffer update batch is missing")

    image_records: dict[tuple[int, int, int], re.Match[str]] = {}
    image_summaries: dict[tuple[int, int, int], re.Match[str]] = {}
    for line in lines:
        match = COMPOSITOR_IMAGE.fullmatch(line)
        if match and match.group("slot") == "1":
            key = (
                int(match.group("generation")),
                int(match.group("ordinal")),
                int(match.group("image_ordinal")),
            )
            image_records[key] = match
        match = COMPOSITOR_SUMMARY.fullmatch(line)
        if match and match.group("slot") == "1":
            key = (
                int(match.group("generation")),
                int(match.group("ordinal")),
                int(match.group("image_ordinal")),
            )
            image_summaries[key] = match
    for frame in compared:
        for image_ordinal in (0, 1):
            identity = image_records.get((*frame, image_ordinal))
            summary = image_summaries.get((*frame, image_ordinal))
            if not identity or not summary:
                reasons.append("a compared frame lacks a sampled compositor input")
                continue
            if (
                identity.group("binding") != summary.group("binding")
                or identity.group("array") != summary.group("array")
                or int(summary.group("samples")) != 5
            ):
                reasons.append("a compositor input sample has inconsistent identity")
    for image_ordinal in (0, 1):
        bindings = {
            (
                image_records[(*frame, image_ordinal)].group("binding"),
                image_records[(*frame, image_ordinal)].group("array"),
            )
            for frame in compared
            if (*frame, image_ordinal) in image_records
        }
        if len(bindings) != 1:
            reasons.append("a compositor input binding changes across compared frames")

    print(f"startup-compositor-audit-run: {run_id}")
    print(f"startup-compositor-audit-pink-observed: {'yes' if pink_observed else 'no'}")
    print(f"startup-compositor-audit-magenta-frames: {sorted(magenta)}")
    print(f"startup-compositor-audit-black-frames: {sorted(black)}")
    print(f"startup-compositor-audit-scene-frames: {sorted(scene)}")
    if reasons:
        return "INCONCLUSIVE", list(dict.fromkeys(reasons))

    image_magenta = _values(classes, magenta, 1, "image_signature")
    image_scene = _values(classes, scene, 1, "image_signature")
    buffer_magenta = {
        slot: _values(classes, magenta, slot, "buffer_signature")
        for slot in TARGET_LAYOUTS
    }
    buffer_scene = {
        slot: _values(classes, scene, slot, "buffer_signature")
        for slot in TARGET_LAYOUTS
    }
    print(f"startup-compositor-audit-magenta-images: {image_magenta}")
    print(f"startup-compositor-audit-scene-images: {image_scene}")
    for slot in TARGET_LAYOUTS:
        print(f"startup-compositor-audit-magenta-set-{slot}-buffers: {buffer_magenta[slot]}")
        print(f"startup-compositor-audit-scene-set-{slot}-buffers: {buffer_scene[slot]}")

    input_names = {0: "SCENE", 1: "GUI"}
    direct_magenta: list[int] = []
    for image_ordinal in (0, 1):
        magenta_near = [
            int(image_summaries[(*frame, image_ordinal)].group("near"))
            for frame in magenta
        ]
        scene_near = [
            int(image_summaries[(*frame, image_ordinal)].group("near"))
            for frame in scene
        ]
        magenta_signatures = sorted({
            image_records[(*frame, image_ordinal)].group("signature")
            for frame in magenta
        })
        scene_signatures = sorted({
            image_records[(*frame, image_ordinal)].group("signature")
            for frame in scene
        })
        print(
            f"startup-compositor-audit-{input_names[image_ordinal].lower()}-"
            f"magenta-near: {magenta_near}"
        )
        print(
            f"startup-compositor-audit-{input_names[image_ordinal].lower()}-"
            f"scene-near: {scene_near}"
        )
        print(
            f"startup-compositor-audit-{input_names[image_ordinal].lower()}-"
            f"magenta-signatures: {magenta_signatures}"
        )
        print(
            f"startup-compositor-audit-{input_names[image_ordinal].lower()}-"
            f"scene-signatures: {scene_signatures}"
        )
        if magenta_near and all(value > 0 for value in magenta_near) and not any(scene_near):
            direct_magenta.append(image_ordinal)

    if len(direct_magenta) == 1:
        image_ordinal = direct_magenta[0]
        magenta_signatures = {
            image_records[(*frame, image_ordinal)].group("signature")
            for frame in magenta
        }
        scene_signatures = {
            image_records[(*frame, image_ordinal)].group("signature")
            for frame in scene
        }
        transition = (
            "DESCRIPTOR-CHANGE"
            if magenta_signatures != scene_signatures
            else "IN-PLACE-CONTENT-CHANGE"
        )
        return f"COMPOSITOR-{input_names[image_ordinal]}-MAGENTA-{transition}", []
    if len(direct_magenta) > 1:
        return "COMPOSITOR-MULTIPLE-MAGENTA-INPUTS", []
    return "COMPOSITOR-COMBINED-INPUT-CANDIDATE", []


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--pink-observed", required=True, choices=("yes", "no"))
    args = parser.parse_args()
    verdict, reasons = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.pink_observed == "yes",
    )
    print(f"startup-compositor-audit-verdict: {verdict}")
    for reason in reasons:
        print(f"startup-compositor-audit-reason: {reason}")
    return 0 if verdict not in {"INVALID", "INCONCLUSIVE"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
