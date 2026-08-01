#!/usr/bin/env python3
"""Correlate ESO's first two surface resets with bridge lifecycle evidence."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


BRIDGE_LINE = re.compile(r"^\[run=([^]]+)] (.*)$")
SWAPCHAIN_CREATE = re.compile(
    r"^SWAPCHAIN_CREATE: .* old=(?P<old>\S+) old_generation=(?P<old_gen>\d+) "
    r"extent=(?P<width>\d+)x(?P<height>\d+) min_images=(?P<min_images>\d+) "
    r"format=(?P<format>\d+) color_space=(?P<color_space>\d+) "
    r"present_mode=(?P<present_mode>\d+) result=(?P<result>-?\d+) "
    r"swapchain=(?P<swapchain>\S+) generation=(?P<generation>\d+)$"
)
SWAPCHAIN_ACQUIRE = re.compile(
    r"^SWAPCHAIN_ACQUIRE: .* generation=(?P<generation>\d+) "
    r"ordinal=(?P<ordinal>\d+) image_index=(?P<image>\d+) "
    r"result=(?P<result>-?\d+)$"
)
SWAPCHAIN_PRESENT = re.compile(
    r"^SWAPCHAIN_PRESENT: .* generation=(?P<generation>\d+) "
    r"ordinal=(?P<ordinal>\d+) image_index=(?P<image>\d+) "
    r"result=(?P<result>-?\d+) item_result=(?P<item_result>-?\d+)$"
)
CLIENT_LINE = re.compile(r"^(?P<timestamp>\S+) .* (?P<event>DeviceWaitIdle|fpCreateSwapchainKHR|OnDeviceReset):")
INTERFACE_LINE = re.compile(
    r"^(?P<timestamp>\S+) .*ZO_PregameStateManager_SetState - from: nil, to: AccountLogin$"
)


@dataclass(frozen=True)
class Swapchain:
    generation: int
    old_generation: int
    width: int
    height: int
    min_images: int
    format: int
    color_space: int
    present_mode: int
    result: int


@dataclass(frozen=True)
class LifecycleResult:
    run_id: str
    first: Swapchain | None
    second: Swapchain | None
    first_acquires: int
    first_presents: int
    reasons: tuple[str, ...]

    @property
    def matches_transient_surface(self) -> bool:
        return not self.reasons


@dataclass(frozen=True)
class TimingResult:
    first_ready: datetime | None
    correction_started: datetime | None
    corrected_ready: datetime | None
    account_login: datetime | None
    reasons: tuple[str, ...]

    @property
    def transient_ms(self) -> float | None:
        if self.first_ready is None or self.correction_started is None:
            return None
        return (self.correction_started - self.first_ready).total_seconds() * 1000

    @property
    def convergence_ms(self) -> float | None:
        if self.first_ready is None or self.corrected_ready is None:
            return None
        return (self.corrected_ready - self.first_ready).total_seconds() * 1000


def _timestamp(value: str) -> datetime:
    return datetime.fromisoformat(value.replace("Z", "+00:00"))


def analyze_lifecycle(text: str, run_id: str) -> LifecycleResult:
    creates: list[Swapchain] = []
    acquires: dict[int, list[re.Match[str]]] = {}
    presents: dict[int, list[re.Match[str]]] = {}
    for raw_line in text.splitlines():
        tagged = BRIDGE_LINE.match(raw_line)
        if not tagged or tagged.group(1) != run_id:
            continue
        message = tagged.group(2)
        if match := SWAPCHAIN_CREATE.match(message):
            creates.append(
                Swapchain(
                    generation=int(match.group("generation")),
                    old_generation=int(match.group("old_gen")),
                    width=int(match.group("width")),
                    height=int(match.group("height")),
                    min_images=int(match.group("min_images")),
                    format=int(match.group("format")),
                    color_space=int(match.group("color_space")),
                    present_mode=int(match.group("present_mode")),
                    result=int(match.group("result")),
                )
            )
        elif match := SWAPCHAIN_ACQUIRE.match(message):
            acquires.setdefault(int(match.group("generation")), []).append(match)
        elif match := SWAPCHAIN_PRESENT.match(message):
            presents.setdefault(int(match.group("generation")), []).append(match)

    reasons: list[str] = []
    first = creates[0] if creates else None
    second = creates[1] if len(creates) > 1 else None
    if first is None or second is None:
        reasons.append("fewer than two swapchains were recorded for the selected run")
        return LifecycleResult(run_id, first, second, 0, 0, tuple(reasons))

    first_acquires = acquires.get(first.generation, [])
    first_presents = presents.get(first.generation, [])
    if first.result != 0 or second.result != 0:
        reasons.append("one of the first two swapchain creations failed")
    if first.old_generation != 0:
        reasons.append("the first swapchain did not start a fresh lifecycle")
    if second.old_generation != first.generation:
        reasons.append("the second swapchain did not replace the first generation")
    if (first.width, first.height) != (second.width, second.height + 2):
        reasons.append("the second extent was not the same width and two pixels shorter")
    if (
        first.min_images,
        first.format,
        first.color_space,
        first.present_mode,
    ) != (
        second.min_images,
        second.format,
        second.color_space,
        second.present_mode,
    ):
        reasons.append("swapchain parameters other than height changed")
    if len(first_acquires) != 1 or int(first_acquires[0].group("result")) != 0:
        reasons.append("the first generation did not have exactly one successful acquire")
    if (
        len(first_presents) != 1
        or int(first_presents[0].group("result")) != 0
        or int(first_presents[0].group("item_result")) != 0
    ):
        reasons.append("the first generation did not have exactly one successful present")

    return LifecycleResult(
        run_id,
        first,
        second,
        len(first_acquires),
        len(first_presents),
        tuple(reasons),
    )


def analyze_timing(client_text: str, interface_text: str = "") -> TimingResult:
    events: list[tuple[datetime, str]] = []
    for raw_line in client_text.splitlines():
        if match := CLIENT_LINE.match(raw_line):
            events.append((_timestamp(match.group("timestamp")), match.group("event")))

    reset_ready = [timestamp for timestamp, event in events if event == "OnDeviceReset"]
    wait_idle = [timestamp for timestamp, event in events if event == "DeviceWaitIdle"]
    reasons: list[str] = []
    first_ready = reset_ready[0] if reset_ready else None
    corrected_ready = reset_ready[1] if len(reset_ready) > 1 else None
    correction_started = None
    if first_ready is not None:
        correction_started = next(
            (timestamp for timestamp in wait_idle if timestamp > first_ready), None
        )
    if first_ready is None or corrected_ready is None:
        reasons.append("the client log lacks the first two completed device resets")
    if correction_started is None:
        reasons.append("the client log lacks the wait that starts surface correction")

    account_login = None
    for raw_line in interface_text.splitlines():
        if match := INTERFACE_LINE.match(raw_line):
            account_login = _timestamp(match.group("timestamp"))
            break
    if account_login is not None and corrected_ready is not None:
        if account_login <= corrected_ready:
            reasons.append("AccountLogin did not begin after surface convergence")

    return TimingResult(
        first_ready,
        correction_started,
        corrected_ready,
        account_login,
        tuple(reasons),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bridge-log", type=Path)
    parser.add_argument("--run-id")
    parser.add_argument("--client-log", type=Path, required=True)
    parser.add_argument("--interface-log", type=Path)
    args = parser.parse_args()
    if bool(args.bridge_log) != bool(args.run_id):
        parser.error("--bridge-log and --run-id must be supplied together")

    timing = analyze_timing(
        args.client_log.read_text(errors="replace"),
        args.interface_log.read_text(errors="replace") if args.interface_log else "",
    )
    if args.bridge_log:
        lifecycle = analyze_lifecycle(
            args.bridge_log.read_text(errors="replace"), args.run_id
        )
        if lifecycle.first and lifecycle.second:
            print(
                "surface: "
                f"generation {lifecycle.first.generation} "
                f"{lifecycle.first.width}x{lifecycle.first.height} -> "
                f"generation {lifecycle.second.generation} "
                f"{lifecycle.second.width}x{lifecycle.second.height}"
            )
        print(
            "first-generation: "
            f"acquires={lifecycle.first_acquires} presents={lifecycle.first_presents}"
        )
        print(
            "lifecycle-verdict: "
            + ("TRANSIENT-SURFACE" if lifecycle.matches_transient_surface else "INCONCLUSIVE")
        )
        for reason in lifecycle.reasons:
            print(f"lifecycle-reason: {reason}")

    if timing.transient_ms is not None:
        print(f"first-ready-to-correction: {timing.transient_ms:.3f} ms")
    if timing.convergence_ms is not None:
        print(f"first-ready-to-corrected-ready: {timing.convergence_ms:.3f} ms")
    if timing.account_login is not None and timing.corrected_ready is not None:
        delay = (timing.account_login - timing.corrected_ready).total_seconds() * 1000
        print(f"corrected-ready-to-account-login: {delay:.3f} ms")
    print("timing-verdict: " + ("CONVERGED" if not timing.reasons else "INCONCLUSIVE"))
    for reason in timing.reasons:
        print(f"timing-reason: {reason}")

    lifecycle_failed = args.bridge_log and not lifecycle.matches_transient_surface
    return 1 if lifecycle_failed or timing.reasons else 0


if __name__ == "__main__":
    raise SystemExit(main())
