#!/usr/bin/env python3
"""Summarize swapchain-generation lifecycle traces from a bridge log."""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from pathlib import Path
import re

from check_startup_log import parse_runs, run_epoch


FIELD = re.compile(r"(?P<key>[a-z_]+)=(?P<value>\S+)")


@dataclass
class Generation:
    number: int
    extent: str = "unknown"
    old_generation: int = 0
    create_result: int = 0
    images: set[str] = field(default_factory=set)
    views_created: int = 0
    views_destroyed: int = 0
    framebuffers_created: int = 0
    framebuffers_destroyed: int = 0
    render_passes: set[str] = field(default_factory=set)
    max_acquire_ordinal: int = 0
    max_present_ordinal: int = 0
    destroyed: bool = False
    destroy_live_views: int = 0
    destroy_live_framebuffers: int = 0


def fields(line: str) -> dict[str, str]:
    return {
        match.group("key"): match.group("value")
        for match in FIELD.finditer(line)
    }


def integer(values: dict[str, str], key: str, default: int = 0) -> int:
    try:
        return int(values.get(key, str(default)))
    except ValueError:
        return default


def summarize(lines: list[str]) -> tuple[dict[int, Generation], list[str]]:
    generations: dict[int, Generation] = {}
    anomalies: list[str] = []
    for line in lines:
        values = fields(line)
        generation = integer(values, "generation")
        if line.startswith("LIFECYCLE_ERROR:"):
            anomalies.append(line)
            continue
        if line.startswith("SWAPCHAIN_CREATE:"):
            if generation == 0:
                anomalies.append(f"untracked or failed swapchain create: {line}")
                continue
            record = generations.setdefault(generation, Generation(generation))
            record.extent = values.get("extent", "unknown")
            record.old_generation = integer(values, "old_generation")
            record.create_result = integer(values, "result")
        elif line.startswith("SWAPCHAIN_IMAGE:") and generation:
            generations.setdefault(
                generation, Generation(generation)
            ).images.add(values.get("image", "unknown"))
        elif line.startswith("SWAPCHAIN_IMAGE_VIEW_CREATE:") and generation:
            generations.setdefault(
                generation, Generation(generation)
            ).views_created += 1
        elif line.startswith("SWAPCHAIN_IMAGE_VIEW_DESTROY:") and generation:
            generations.setdefault(
                generation, Generation(generation)
            ).views_destroyed += 1
        elif line.startswith("SWAPCHAIN_FRAMEBUFFER_CREATE:") and generation:
            record = generations.setdefault(generation, Generation(generation))
            record.framebuffers_created += 1
            record.render_passes.add(values.get("render_pass", "unknown"))
            if values.get("mixed_generations") == "yes":
                anomalies.append(
                    f"generation {generation} framebuffer mixed image generations"
                )
        elif line.startswith("SWAPCHAIN_FRAMEBUFFER_DESTROY:") and generation:
            generations.setdefault(
                generation, Generation(generation)
            ).framebuffers_destroyed += 1
        elif line.startswith("SWAPCHAIN_ACQUIRE:") and generation:
            record = generations.setdefault(generation, Generation(generation))
            record.max_acquire_ordinal = max(
                record.max_acquire_ordinal, integer(values, "ordinal")
            )
            if integer(values, "result") not in {
                0,
                1000001003,  # VK_SUBOPTIMAL_KHR
            }:
                anomalies.append(
                    f"generation {generation} acquire result={values.get('result')}"
                )
        elif line.startswith("SWAPCHAIN_PRESENT:") and generation:
            record = generations.setdefault(generation, Generation(generation))
            record.max_present_ordinal = max(
                record.max_present_ordinal, integer(values, "ordinal")
            )
            if integer(values, "result") not in {
                0,
                1000001003,  # VK_SUBOPTIMAL_KHR
            }:
                anomalies.append(
                    f"generation {generation} present result={values.get('result')}"
                )
            if integer(values, "item_result") not in {
                0,
                1000001003,
            }:
                anomalies.append(
                    "generation "
                    f"{generation} item_result={values.get('item_result')}"
                )
        elif line.startswith("SWAPCHAIN_DESTROY:") and generation:
            record = generations.setdefault(generation, Generation(generation))
            record.destroyed = True
            record.destroy_live_views = integer(values, "live_views")
            record.destroy_live_framebuffers = integer(
                values, "live_framebuffers"
            )
            if record.destroy_live_views or record.destroy_live_framebuffers:
                anomalies.append(
                    f"generation {generation} destroyed with "
                    f"live_views={record.destroy_live_views} "
                    f"live_framebuffers={record.destroy_live_framebuffers}"
                )
    return generations, anomalies


def command_analyze(args: argparse.Namespace) -> int:
    text = args.log.read_text(encoding="utf-8", errors="replace")
    eligible: list[tuple[float, int, str, list[str]]] = []
    for order, (run_id, lines) in enumerate(parse_runs(text).items()):
        epoch = run_epoch(run_id)
        if args.after_epoch is not None and (
            epoch is None or epoch < args.after_epoch
        ):
            continue
        if any(line.startswith("SWAPCHAIN_CREATE:") for line in lines):
            eligible.append(
                (epoch if epoch is not None else float("-inf"),
                 order, run_id, lines)
            )
    if not eligible:
        print("lifecycle run: none")
        print("lifecycle trace: MISSING")
        return 2

    _, _, run_id, lines = max(eligible)
    generations, anomalies = summarize(lines)
    print(f"lifecycle run: {run_id}")
    print(f"swapchain generations: {len(generations)}")
    for number in sorted(generations):
        record = generations[number]
        print(
            f"generation {number}: extent={record.extent} "
            f"old_generation={record.old_generation} "
            f"images={len(record.images)} "
            f"views={record.views_created}/{record.views_destroyed} "
            f"framebuffers="
            f"{record.framebuffers_created}/{record.framebuffers_destroyed} "
            f"render_passes={len(record.render_passes)} "
            f"acquires={record.max_acquire_ordinal} "
            f"presents={record.max_present_ordinal} "
            f"destroyed={'yes' if record.destroyed else 'no'}"
        )
    print(f"lifecycle anomalies: {len(anomalies)}")
    for anomaly in anomalies:
        print(f"- {anomaly}")
    return 0


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("log", type=Path)
    value.add_argument("--after-epoch", type=float)
    value.set_defaults(function=command_analyze)
    return value


def main() -> int:
    args = parser().parse_args()
    try:
        return args.function(args)
    except OSError as error:
        print(f"ERROR: {error}")
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
