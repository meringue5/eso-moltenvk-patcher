#!/usr/bin/env python3
"""Summarize one bounded render-graph audit from a bridge log."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from analyze_reset_resource_log import fields, number
from check_startup_log import parse_runs, run_epoch


EXPECTED_COUNTERS = {
    "descriptor_update_calls",
    "descriptor_image_writes",
    "descriptor_copies",
    "descriptor_multi_writes",
    "descriptor_unknown_views_written",
    "descriptor_dead_views_written",
    "descriptor_views_destroyed_while_referenced",
    "descriptor_set_bind_calls",
    "descriptor_sets_bound",
    "descriptor_unknown_sets_bound",
    "descriptor_stale_sets_bound",
    "descriptor_stale_slots_bound",
    "image_binds",
    "image_dead_range_reuses",
    "image_live_overlaps",
    "image_undeclared_overlaps",
    "pipeline_barrier_calls",
    "image_barriers",
    "layout_mismatches",
    "render_pass_begins",
    "attachment_samples",
    "unknown_attachment_views",
    "dead_attachment_views",
    "pipeline_binds",
    "pipeline_binds_known",
    "pipeline_binds_created_during_audit",
    "pipeline_render_pass_exact",
    "pipeline_render_pass_different",
    "graphics_pipelines_created",
    "graphics_pipelines_created_during_audit",
    "graphics_pipelines_with_cache",
    "copy_image_calls",
    "blit_image_calls",
    "resolve_image_calls",
    "state_overflows",
}


@dataclass(frozen=True)
class Summary:
    run_id: str | None
    complete: bool
    reason: str
    counters: dict[str, int]
    findings: tuple[str, ...]
    anomalies: tuple[str, ...]


def summarize_run(run_id: str, lines: list[str]) -> Summary:
    begins = [
        line for line in lines if line.startswith("RENDER_AUDIT_BEGIN:")
    ]
    summaries = [
        line for line in lines if line.startswith("RENDER_AUDIT_SUMMARY:")
    ]
    counter_lines = [
        line for line in lines if line.startswith("RENDER_AUDIT_COUNT:")
    ]
    anomalies: list[str] = []
    if len(begins) != 1:
        anomalies.append(f"expected one audit begin, found {len(begins)}")
    elif fields(begins[0]).get("mirror") != "enabled":
        anomalies.append("descriptor mirror was not enabled")
    if len(summaries) != 1:
        anomalies.append(
            f"expected one audit summary, found {len(summaries)}"
        )

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
    if counters.get("state_overflows", 0) != 0:
        anomalies.append(
            f"state mirror overflowed {counters['state_overflows']} time(s)"
        )

    def total(*names: str) -> int:
        return sum(counters.get(name, 0) for name in names)

    findings: list[str] = []
    stale = total(
        "descriptor_dead_views_written",
        "descriptor_views_destroyed_while_referenced",
        "descriptor_stale_sets_bound",
        "descriptor_stale_slots_bound",
    )
    findings.append(
        "descriptor-lifetime: "
        + (
            f"SIGNAL ({stale} stale/dead-reference events)"
            if stale
            else "no stale/dead-reference signal in the bounded window"
        )
    )
    unknown = total(
        "descriptor_unknown_views_written",
        "descriptor_unknown_sets_bound",
        "unknown_attachment_views",
    )
    findings.append(
        "descriptor-mirror-coverage: "
        + (
            f"PARTIAL ({unknown} unknown handle events)"
            if unknown
            else "complete for observed handle use"
        )
    )
    overlap = counters.get("image_undeclared_overlaps", 0)
    dead_reuse = counters.get("image_dead_range_reuses", 0)
    findings.append(
        "image-memory-reuse: "
        f"dead-range-reuses={dead_reuse}; live-overlap="
        + (
            f"SIGNAL ({overlap} undeclared overlap events)"
            if overlap
            else "no undeclared live overlap observed"
        )
    )
    layout = counters.get("layout_mismatches", 0)
    findings.append(
        "layout-transition: "
        + (
            f"SIGNAL ({layout} tracked/declaration mismatches)"
            if layout
            else "no tracked layout mismatch observed"
        )
    )
    created = counters.get(
        "graphics_pipelines_created_during_audit", 0
    )
    rebound = counters.get("pipeline_binds_created_during_audit", 0)
    different = counters.get("pipeline_render_pass_different", 0)
    findings.append(
        "pipeline-reset-linkage: "
        f"created={created} bound_reset_created={rebound} "
        f"compatible-or-different-renderpass-binds={different}"
    )
    transfer = total(
        "copy_image_calls", "blit_image_calls", "resolve_image_calls"
    )
    findings.append(f"image-transfer-path: calls={transfer}")

    summary_fields = fields(summaries[0]) if len(summaries) == 1 else {}
    return Summary(
        run_id=run_id,
        complete=(
            len(summaries) == 1
            and summary_fields.get("complete") == "yes"
        ),
        reason=summary_fields.get("reason", "missing"),
        counters=counters,
        findings=tuple(findings),
        anomalies=tuple(anomalies),
    )


def analyze(text: str, after_epoch: float | None = None) -> Summary:
    eligible = eligible_runs(text, after_epoch)
    if not eligible:
        return Summary(
            None,
            False,
            "missing",
            {},
            (),
            ("no render audit matched the time gate",),
        )
    _, _, run_id, lines = max(eligible)
    return summarize_run(run_id, lines)


def eligible_runs(
    text: str, after_epoch: float | None = None
) -> list[tuple[float, int, str, list[str]]]:
    eligible: list[tuple[float, int, str, list[str]]] = []
    for order, (run_id, lines) in enumerate(parse_runs(text).items()):
        epoch = run_epoch(run_id)
        if after_epoch is not None and (
            epoch is None or epoch < after_epoch
        ):
            continue
        if any(line.startswith("RENDER_AUDIT_BEGIN:") for line in lines):
            eligible.append(
                (
                    epoch if epoch is not None else float("-inf"),
                    order,
                    run_id,
                    lines,
                )
            )
    return eligible


def analyze_all(
    text: str, after_epoch: float | None = None
) -> tuple[Summary, ...]:
    return tuple(
        summarize_run(run_id, lines)
        for _, _, run_id, lines in eligible_runs(text, after_epoch)
    )


COMPARISON_COUNTERS = (
    "descriptor_update_calls",
    "descriptor_image_writes",
    "descriptor_views_destroyed_while_referenced",
    "descriptor_stale_sets_bound",
    "descriptor_stale_slots_bound",
    "image_dead_range_reuses",
    "image_live_overlaps",
    "layout_mismatches",
    "graphics_pipelines_created_during_audit",
    "graphics_pipelines_with_cache",
    "pipeline_binds_created_during_audit",
    "pipeline_render_pass_exact",
    "pipeline_render_pass_different",
    "render_pass_begins",
    "state_overflows",
)


def command_all_runs(args: argparse.Namespace) -> int:
    results = analyze_all(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.after_epoch,
    )
    print(f"render-audit eligible runs: {len(results)}")
    completed = 0
    invalid_completed = 0
    for index, result in enumerate(results, start=1):
        print(
            f"run {index}: {result.run_id or 'none'} "
            f"complete={'yes' if result.complete else 'no'} "
            f"reason={result.reason}"
        )
        if not result.complete:
            print("- audit ended before a summary; no counter comparison")
            continue
        completed += 1
        invalid_completed += bool(result.anomalies)
        for name in COMPARISON_COUNTERS:
            print(f"- {name}={result.counters.get(name, -1)}")
        for anomaly in result.anomalies:
            print(f"- analyzer anomaly: {anomaly}")
    print(
        f"render-audit comparison: completed={completed} "
        f"invalid_completed={invalid_completed}"
    )
    return 0 if completed > 0 and invalid_completed == 0 else 2


def command_analyze(args: argparse.Namespace) -> int:
    result = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.after_epoch,
    )
    print(f"render-audit run: {result.run_id or 'none'}")
    print(
        "render-audit summary: "
        f"complete={'yes' if result.complete else 'no'} "
        f"reason={result.reason}"
    )
    for finding in result.findings:
        print(f"- {finding}")
    for name in sorted(result.counters):
        print(f"{name}: {result.counters[name]}")
    print(f"render-audit analyzer anomalies: {len(result.anomalies)}")
    for anomaly in result.anomalies:
        print(f"- {anomaly}")
    return 0 if result.complete and not result.anomalies else 2


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("log", type=Path)
    value.add_argument("--after-epoch", type=float)
    value.add_argument("--all-runs", action="store_true")
    value.set_defaults(function=command_analyze)
    return value


def main() -> int:
    args = parser().parse_args()
    if args.all_runs:
        args.function = command_all_runs
    try:
        return args.function(args)
    except OSError as error:
        print(f"ERROR: {error}")
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
