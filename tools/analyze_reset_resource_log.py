#!/usr/bin/env python3
"""Summarize one bounded reset-resource trace from a bridge log."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re

from check_startup_log import parse_runs, run_epoch


FIELD = re.compile(r"(?P<key>[a-z_]+)=(?P<value>\S+)")
EXPECTED_COUNTERS = {
    "allocate_memory",
    "free_memory",
    "map_memory",
    "unmap_memory",
    "create_buffer",
    "destroy_buffer",
    "bind_buffer_memory",
    "get_buffer_memory_requirements",
    "create_image",
    "destroy_image",
    "bind_image_memory",
    "get_image_memory_requirements",
    "create_image_view",
    "destroy_image_view",
    "create_render_pass",
    "destroy_render_pass",
    "create_framebuffer",
    "destroy_framebuffer",
    "create_descriptor_pool",
    "destroy_descriptor_pool",
    "reset_descriptor_pool",
    "allocate_descriptor_sets",
    "free_descriptor_sets",
    "update_descriptor_writes",
    "update_descriptor_copies",
    "create_descriptor_set_layout",
    "destroy_descriptor_set_layout",
    "create_pipeline_layout",
    "destroy_pipeline_layout",
    "create_graphics_pipelines",
    "create_compute_pipelines",
    "destroy_pipeline",
    "allocate_command_buffers",
    "free_command_buffers",
    "begin_command_buffer",
    "end_command_buffer",
    "queue_submit",
    "submitted_command_buffers",
    "cmd_begin_render_pass",
    "cmd_bind_pipeline",
    "cmd_bind_descriptor_sets",
    "cmd_draw",
    "cmd_draw_indexed",
    "cmd_dispatch",
    "cmd_end_render_pass",
}


@dataclass(frozen=True)
class Summary:
    run_id: str | None
    complete: bool
    reason: str
    reset_presents: int
    failures: int
    counters: dict[str, int]
    anomalies: tuple[str, ...]


def fields(line: str) -> dict[str, str]:
    return {
        match.group("key"): match.group("value")
        for match in FIELD.finditer(line)
    }


def number(values: dict[str, str], key: str, default: int = 0) -> int:
    try:
        return int(values.get(key, str(default)))
    except ValueError:
        return default


def summarize_run(run_id: str, lines: list[str]) -> Summary:
    begins = [
        line
        for line in lines
        if line.startswith("RESET_RESOURCE_TRACE_BEGIN:")
    ]
    swapchains = [
        line
        for line in lines
        if line.startswith("RESET_RESOURCE_TRACE_SWAPCHAIN:")
    ]
    summaries = [
        line
        for line in lines
        if line.startswith("RESET_RESOURCE_TRACE_SUMMARY:")
    ]
    failures = [
        line
        for line in lines
        if line.startswith("RESET_RESOURCE_FAILURE:")
    ]
    counter_lines = [
        line
        for line in lines
        if line.startswith("RESET_RESOURCE_COUNT:")
    ]
    anomalies: list[str] = []
    if len(begins) != 1:
        anomalies.append(f"expected one trace begin, found {len(begins)}")
    if len(swapchains) != 1:
        anomalies.append(
            f"expected one reset swapchain, found {len(swapchains)}"
        )
    if len(summaries) != 1:
        anomalies.append(f"expected one trace summary, found {len(summaries)}")

    counters: dict[str, int] = {}
    for line in counter_lines:
        values = fields(line)
        name = values.get("name", "")
        if not name or name in counters:
            anomalies.append(f"malformed or duplicate counter: {line}")
            continue
        counters[name] = number(values, "value", -1)
    missing = EXPECTED_COUNTERS - set(counters)
    unexpected = set(counters) - EXPECTED_COUNTERS
    if missing:
        anomalies.append("missing counters: " + ",".join(sorted(missing)))
    if unexpected:
        anomalies.append(
            "unexpected counters: " + ",".join(sorted(unexpected))
        )
    if any(value < 0 for value in counters.values()):
        anomalies.append("at least one counter has an invalid value")

    summary_fields = fields(summaries[0]) if len(summaries) == 1 else {}
    reported_failures = number(summary_fields, "failures", -1)
    if reported_failures != len(failures):
        anomalies.append(
            "failure summary mismatch: "
            f"reported={reported_failures} records={len(failures)}"
        )
    return Summary(
        run_id=run_id,
        complete=len(summaries) == 1,
        reason=summary_fields.get("reason", "missing"),
        reset_presents=number(summary_fields, "reset_presents", 0),
        failures=max(reported_failures, 0),
        counters=counters,
        anomalies=tuple(anomalies),
    )


def analyze(text: str, after_epoch: float | None = None) -> Summary:
    eligible: list[tuple[float, int, str, list[str]]] = []
    for order, (run_id, lines) in enumerate(parse_runs(text).items()):
        epoch = run_epoch(run_id)
        if after_epoch is not None and (
            epoch is None or epoch < after_epoch
        ):
            continue
        if any(
            line.startswith("RESET_RESOURCE_TRACE_BEGIN:")
            for line in lines
        ):
            eligible.append(
                (
                    epoch if epoch is not None else float("-inf"),
                    order,
                    run_id,
                    lines,
                )
            )
    if not eligible:
        return Summary(
            None, False, "missing", 0, 0, {},
            ("no reset-resource trace matched the time gate",),
        )
    _, _, run_id, lines = max(eligible)
    return summarize_run(run_id, lines)


def command_analyze(args: argparse.Namespace) -> int:
    result = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.after_epoch,
    )
    print(f"reset-resource run: {result.run_id or 'none'}")
    print(
        "reset-resource summary: "
        f"complete={'yes' if result.complete else 'no'} "
        f"reason={result.reason} presents={result.reset_presents} "
        f"failures={result.failures}"
    )
    for name in sorted(result.counters):
        print(f"{name}: {result.counters[name]}")
    print(f"reset-resource anomalies: {len(result.anomalies)}")
    for anomaly in result.anomalies:
        print(f"- {anomaly}")
    return 0 if result.complete and not result.anomalies else 2


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
