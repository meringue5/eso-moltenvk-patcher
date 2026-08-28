#!/usr/bin/env python3
"""Validate and summarize one measured swapchain experiment run."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path

from check_startup_log import parse_runs, run_epoch


SUMMARY_PREFIX = "SWAPCHAIN_EXPERIMENT_SUMMARY: "
CHECKPOINT_PREFIX = "SWAPCHAIN_EXPERIMENT_CHECKPOINT: "
PAIR = re.compile(r"(?P<key>[a-z0-9_]+)=(?P<value>[^ ]+)")


@dataclass(frozen=True)
class Analysis:
    run_id: str | None
    values: dict[str, str]
    reasons: tuple[str, ...]

    @property
    def passed(self) -> bool:
        return self.run_id is not None and not self.reasons


def _integer(values: dict[str, str], key: str, reasons: list[str]) -> int:
    try:
        return int(values[key])
    except (KeyError, ValueError):
        reasons.append(f"missing or invalid {key}")
        return 0


def analyze(
    text: str,
    *,
    after_epoch: float | None = None,
    expected_mode: str | None = None,
    minimum_samples: int = 600,
) -> Analysis:
    eligible: list[tuple[float, int, str, list[str]]] = []
    for order, (run_id, lines) in enumerate(parse_runs(text).items()):
        epoch = run_epoch(run_id)
        if after_epoch is not None and (epoch is None or epoch < after_epoch):
            continue
        if any(
            line.startswith((SUMMARY_PREFIX, CHECKPOINT_PREFIX)) for line in lines
        ):
            eligible.append(
                (epoch if epoch is not None else float("-inf"), order, run_id, lines)
            )
    if not eligible:
        return Analysis(None, {}, ("no completed swapchain experiment matched",))

    _, _, run_id, lines = max(eligible)
    summaries = [line for line in lines if line.startswith(SUMMARY_PREFIX)]
    checkpoints = [line for line in lines if line.startswith(CHECKPOINT_PREFIX)]
    reasons: list[str] = []
    if len(summaries) > 1:
        reasons.append("run contained more than one final experiment summary")
    selected = summaries[-1] if summaries else checkpoints[-1]
    values = {
        match.group("key"): match.group("value")
        for match in PAIR.finditer(selected)
    }
    values["source"] = "summary" if summaries else "checkpoint"
    mode = values.get("mode")
    if mode not in {"control", "triple"}:
        reasons.append("summary mode is not control or triple")
    if expected_mode is not None and mode != expected_mode:
        reasons.append(f"expected mode {expected_mode}, observed {mode or 'missing'}")

    creates = _integer(values, "creates", reasons)
    promoted = _integer(values, "promoted", reasons)
    forwarded = _integer(values, "forwarded", reasons)
    capability_misses = _integer(values, "capability_misses", reasons)
    returned_two = _integer(values, "returned_two", reasons)
    returned_three = _integer(values, "returned_three", reasons)
    count_mismatches = _integer(values, "returned_count_mismatches", reasons)
    acquire_samples = _integer(values, "acquire_samples", reasons)
    present_samples = _integer(values, "present_samples", reasons)
    interval_samples = _integer(values, "interval_samples", reasons)
    acquire_errors = _integer(values, "acquire_errors", reasons)
    present_errors = _integer(values, "present_errors", reasons)

    if creates == 0:
        reasons.append("no swapchain creation was observed")
    if capability_misses:
        reasons.append("a create occurred without observed surface capabilities")
    if count_mismatches:
        reasons.append("MoltenVK returned fewer images than requested")
    if acquire_errors or present_errors:
        reasons.append("acquire or present returned an error")
    if min(acquire_samples, present_samples, interval_samples) < minimum_samples:
        reasons.append(f"fewer than {minimum_samples} post-warmup samples")

    if mode == "control":
        if promoted != 0 or forwarded != creates:
            reasons.append("control did not forward every swapchain unchanged")
        if returned_two == 0 or returned_three != 0:
            reasons.append("control did not prove an exact two-image swapchain")
    elif mode == "triple":
        if promoted != creates or forwarded != 0:
            reasons.append("candidate did not promote every swapchain")
        if returned_three == 0 or returned_two != 0:
            reasons.append("candidate did not prove an exact three-image swapchain")

    return Analysis(run_id, values, tuple(dict.fromkeys(reasons)))


def _rate(values: dict[str, str], key: str) -> str:
    try:
        interval_us = int(values[key])
    except (KeyError, ValueError):
        return "unavailable"
    if interval_us <= 0:
        return "unavailable"
    return f"{1_000_000 / interval_us:.2f}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--after-epoch", type=float)
    parser.add_argument("--expect-mode", choices=("control", "triple"))
    parser.add_argument("--minimum-samples", type=int, default=600)
    args = parser.parse_args()
    result = analyze(
        args.log.read_text(errors="replace"),
        after_epoch=args.after_epoch,
        expected_mode=args.expect_mode,
        minimum_samples=args.minimum_samples,
    )
    print(f"swapchain-experiment-run: {result.run_id or 'none'}")
    print(f"swapchain-experiment-verdict: {'PASS' if result.passed else 'FAIL'}")
    for key in (
        "source", "mode", "creates", "promoted", "forwarded", "returned_two",
        "returned_three", "acquire_samples", "acquire_p95_us",
        "acquire_p99_us", "acquire_p999_us", "present_p95_us",
        "present_p99_us", "present_p999_us", "interval_p95_us",
        "interval_p99_us", "interval_p999_us",
    ):
        if key in result.values:
            print(f"swapchain-experiment-{key.replace('_', '-')}: {result.values[key]}")
    print(f"swapchain-experiment-1pct-low-proxy-fps: {_rate(result.values, 'interval_p99_us')}")
    print(f"swapchain-experiment-0.1pct-low-proxy-fps: {_rate(result.values, 'interval_p999_us')}")
    for reason in result.reasons:
        print(f"swapchain-experiment-reason: {reason}")
    return 0 if result.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
