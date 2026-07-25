#!/usr/bin/env python3
"""Check captured reset-time image-memory bindings for basic invalidity."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path

from analyze_reset_resource_log import fields
from check_startup_log import parse_runs, run_epoch


@dataclass(frozen=True)
class Binding:
    image: str
    memory: str
    offset: int
    size: int
    alignment: int
    flags: int

    @property
    def end(self) -> int:
        return self.offset + self.size


@dataclass(frozen=True)
class BindingAnalysis:
    run_id: str | None
    bindings: tuple[Binding, ...]
    anomalies: tuple[str, ...]
    incomplete_records: int


def integer(value: str, default: int = -1) -> int:
    try:
        return int(value, 0)
    except ValueError:
        return default


def analyze_run(run_id: str, lines: list[str]) -> BindingAnalysis:
    created: dict[str, int] = {}
    requirements: dict[str, tuple[int, int]] = {}
    pending_image: str | None = None
    bindings: list[Binding] = []
    anomalies: list[str] = []

    for line in lines:
        if not line.startswith("RESET_RESOURCE_DETAIL:"):
            continue
        values = fields(line)
        operation = values.get("operation")
        if operation == "create_image":
            image = values.get("image", "")
            result = integer(values.get("result", "-1"))
            if result == 0 and image:
                created[image] = integer(values.get("flags", "-1"))
                pending_image = image
        elif operation == "get_image_memory_requirements":
            if pending_image is not None:
                requirements[pending_image] = (
                    integer(values.get("size", "-1")),
                    integer(values.get("alignment", "-1")),
                )
        elif operation == "bind_image_memory":
            image = values.get("image", "")
            memory = values.get("memory", "")
            result = integer(values.get("result", "-1"))
            requirement = requirements.get(image)
            if result == 0 and image in created and memory and requirement:
                size, alignment = requirement
                binding = Binding(
                    image=image,
                    memory=memory,
                    offset=integer(values.get("offset", "-1")),
                    size=size,
                    alignment=alignment,
                    flags=created[image],
                )
                bindings.append(binding)
                if binding.offset < 0 or binding.size <= 0:
                    anomalies.append(f"{image}: invalid offset or size")
                if binding.alignment <= 0:
                    anomalies.append(f"{image}: invalid alignment")
                elif binding.offset % binding.alignment:
                    anomalies.append(
                        f"{image}: offset {binding.offset} is not aligned "
                        f"to {binding.alignment}"
                    )
            if image == pending_image:
                pending_image = None

    grouped: dict[str, list[Binding]] = {}
    for binding in bindings:
        grouped.setdefault(binding.memory, []).append(binding)
    for memory, members in grouped.items():
        ordered = sorted(members, key=lambda item: (item.offset, item.end))
        for left, right in zip(ordered, ordered[1:]):
            if right.offset < left.end:
                anomalies.append(
                    f"{memory}: captured images {left.image} and {right.image} "
                    f"overlap [{right.offset},{min(left.end, right.end)})"
                )

    incomplete = sum(
        1 for image in created if not any(item.image == image for item in bindings)
    )
    return BindingAnalysis(
        run_id=run_id,
        bindings=tuple(bindings),
        anomalies=tuple(anomalies),
        incomplete_records=incomplete,
    )


def analyze(text: str, after_epoch: float | None = None) -> BindingAnalysis:
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
        return BindingAnalysis(
            None, (), ("no reset-resource trace matched the time gate",), 0
        )
    _, _, run_id, lines = max(eligible)
    return analyze_run(run_id, lines)


def command_analyze(args: argparse.Namespace) -> int:
    result = analyze(
        args.log.read_text(encoding="utf-8", errors="replace"),
        args.after_epoch,
    )
    memory_count = len({item.memory for item in result.bindings})
    print(f"reset-binding run: {result.run_id or 'none'}")
    print(
        f"captured image bindings: {len(result.bindings)} "
        f"memories={memory_count} incomplete={result.incomplete_records}"
    )
    print(f"reset-binding anomalies: {len(result.anomalies)}")
    for anomaly in result.anomalies:
        print(f"- {anomaly}")
    return 0 if result.run_id and not result.anomalies else 2


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
