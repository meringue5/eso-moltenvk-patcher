#!/usr/bin/env python3

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import statistics
import sys


SAMPLE = re.compile(
    r"^descriptor benchmark sample=(?P<sample>[0-9]+) "
    r"draws=(?P<draws>[0-9]+) "
    r"submit_ns=(?P<submit>[0-9]+) "
    r"wait_ns=(?P<wait>[0-9]+)$"
)
PASS = re.compile(
    r"^descriptor benchmark: PASS draws=(?P<draws>[0-9]+) "
    r"samples=(?P<samples>[0-9]+) alternating_resources=yes$"
)


@dataclass(frozen=True)
class Run:
    path: Path
    draws: int
    submit_ns: tuple[int, ...]
    wait_ns: tuple[int, ...]

    @property
    def median_submit_ns(self) -> float:
        return statistics.median(self.submit_ns)

    @property
    def median_submit_ns_per_draw(self) -> float:
        return self.median_submit_ns / self.draws


def parse_run(path: Path) -> Run:
    samples: list[tuple[int, int, int, int]] = []
    pass_records: list[tuple[int, int]] = []
    text = path.read_text(errors="replace")
    if "match=NO" in text:
        raise ValueError(f"{path}: pixel validation failed")
    for line in text.splitlines():
        sample_match = SAMPLE.fullmatch(line)
        if sample_match:
            samples.append(
                (
                    int(sample_match.group("sample")),
                    int(sample_match.group("draws")),
                    int(sample_match.group("submit")),
                    int(sample_match.group("wait")),
                )
            )
            continue
        pass_match = PASS.fullmatch(line)
        if pass_match:
            pass_records.append(
                (
                    int(pass_match.group("draws")),
                    int(pass_match.group("samples")),
                )
            )
    if len(pass_records) != 1:
        raise ValueError(f"{path}: expected exactly one benchmark PASS record")
    pass_draws, pass_samples = pass_records[0]
    if len(samples) != pass_samples:
        raise ValueError(
            f"{path}: sample count {len(samples)} != PASS {pass_samples}"
        )
    if [sample[0] for sample in samples] != list(range(pass_samples)):
        raise ValueError(f"{path}: sample ordinals are incomplete")
    if any(sample[1] != pass_draws for sample in samples):
        raise ValueError(f"{path}: inconsistent draw count")
    if pass_draws < 2 or any(sample[2] <= 0 or sample[3] < 0 for sample in samples):
        raise ValueError(f"{path}: invalid timing data")
    return Run(
        path=path,
        draws=pass_draws,
        submit_ns=tuple(sample[2] for sample in samples),
        wait_ns=tuple(sample[3] for sample in samples),
    )


def classify(live_on: list[Run], live_off: list[Run]) -> tuple[str, float]:
    if not live_on or not live_off:
        raise ValueError("both live-on and live-off runs are required")
    draws = {run.draws for run in live_on + live_off}
    if len(draws) != 1:
        raise ValueError("all runs must use the same draw count")
    on_median = statistics.median(
        value
        for run in live_on
        for value in run.submit_ns
    )
    off_median = statistics.median(
        value
        for run in live_off
        for value in run.submit_ns
    )
    reduction = (on_median - off_median) * 100.0 / on_median
    if reduction >= 3.0:
        return "MEASURED_GAIN", reduction
    if reduction > 0:
        return "NEGLIGIBLE_GAIN", reduction
    return "NO_GAIN", reduction


def format_group(label: str, runs: list[Run]) -> list[str]:
    all_submit = [
        value
        for run in runs
        for value in run.submit_ns
    ]
    draw_count = runs[0].draws
    return [
        (
            f"{label}: runs={len(runs)} samples={len(all_submit)} "
            f"draws={draw_count}"
        ),
        (
            f"{label}: aggregate_median_submit_ns="
            f"{statistics.median(all_submit):.1f} "
            f"ns_per_draw={statistics.median(all_submit) / draw_count:.3f}"
        ),
        (
            f"{label}: run_median_ns_per_draw="
            + ",".join(
                f"{run.median_submit_ns_per_draw:.3f}"
                for run in runs
            )
        ),
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--live-on", type=Path, action="append", required=True
    )
    parser.add_argument(
        "--live-off", type=Path, action="append", required=True
    )
    args = parser.parse_args()
    try:
        live_on = [parse_run(path) for path in args.live_on]
        live_off = [parse_run(path) for path in args.live_off]
        classification, reduction = classify(live_on, live_off)
    except (OSError, ValueError) as error:
        print(f"descriptor benchmark: INCONCLUSIVE: {error}")
        return 2

    for line in format_group("live_on", live_on):
        print(line)
    for line in format_group("live_off", live_off):
        print(line)
    print(f"submit_time_reduction_percent={reduction:.3f}")
    print(f"descriptor benchmark classification: {classification}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
