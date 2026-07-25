#!/usr/bin/env python3
"""Extract ESO Vulkan live-reset sequences from the client log."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re


EVENT = re.compile(
    r"^(?P<timestamp>\S+) \[zos\]\[ZoRenderDeviceVk\] "
    r"(?P<name>DeviceWaitIdle|fpCreateSwapchainKHR|OnDeviceReset): "
    r"(?P<duration>[0-9.]+) ms$"
)
ERROR = re.compile(
    r"error|warn|failed|device.?lost|command.?buffer",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class ResetEvent:
    timestamp: str
    wait_count: int
    longest_wait_ms: float | None
    swapchain_ms: float | None
    reset_ms: float
    terminal: bool


def analyze(text: str) -> tuple[list[ResetEvent], int]:
    lines = [line for line in text.splitlines() if line]
    parsed = []
    for index, line in enumerate(lines):
        match = EVENT.fullmatch(line)
        if match:
            parsed.append(
                (
                    index,
                    match.group("timestamp"),
                    match.group("name"),
                    float(match.group("duration")),
                )
            )

    events: list[ResetEvent] = []
    waits: list[float] = []
    swapchain: float | None = None
    for index, timestamp, name, duration in parsed:
        if name == "DeviceWaitIdle":
            waits.append(duration)
        elif name == "fpCreateSwapchainKHR":
            swapchain = duration
        else:
            events.append(
                ResetEvent(
                    timestamp=timestamp,
                    wait_count=len(waits),
                    longest_wait_ms=max(waits) if waits else None,
                    swapchain_ms=swapchain,
                    reset_ms=duration,
                    terminal=index == len(lines) - 1,
                )
            )
            waits = []
            swapchain = None
    error_count = sum(1 for line in lines if ERROR.search(line))
    return events, error_count


def duration(value: float | None) -> str:
    return "missing" if value is None else f"{value:.6f}"


def command_analyze(args: argparse.Namespace) -> int:
    text = args.log.read_text(encoding="utf-8", errors="replace")
    events, error_count = analyze(text)
    print(f"Reset events: {len(events)}")
    print(f"Error markers: {error_count}")
    for index, event in enumerate(events, 1):
        print(
            f"Reset {index}: timestamp={event.timestamp} "
            f"wait_count={event.wait_count} "
            f"longest_wait_ms={duration(event.longest_wait_ms)} "
            f"swapchain_ms={duration(event.swapchain_ms)} "
            f"reset_ms={event.reset_ms:.6f} "
            f"terminal={'yes' if event.terminal else 'no'}"
        )
    return 0


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("log", type=Path)
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
