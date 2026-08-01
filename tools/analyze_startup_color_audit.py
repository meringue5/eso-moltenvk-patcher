#!/usr/bin/env python3
"""Classify an exact bounded two-generation startup color audit run."""

from __future__ import annotations

import argparse
import re
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


TAGGED = re.compile(r"^\[run=([^]]+)] (.*)$")
VK_SUCCESS = 0
VK_SUBOPTIMAL_KHR = 1000001003
CLEAR = re.compile(
    r"^STARTUP_COLOR_CLEAR: generation=(?P<generation>[12]) "
    r".* framebuffer=(?P<framebuffer>\S+) "
    r".* rgba=(?P<rgba>[^ ]+) .* rect_count=(?P<rects>\d+)$"
)
BEGIN_CLEAR = re.compile(
    r"^STARTUP_COLOR_BEGIN_CLEAR: generation=(?P<generation>[12]) "
    r".* framebuffer=(?P<framebuffer>\S+) "
    r"format=\d+ load_op=(?P<load_op>\d+) rgba=(?P<rgba>[^ ]+) "
)
SUBMIT = re.compile(
    r"^STARTUP_COLOR_SUBMIT: generation=(?P<generation>[12]) "
    r"queue=(?P<queue>\S+) .* "
    r"framebuffer=(?P<framebuffer>\S+) .* result=(?P<result>-?\d+)$"
)
PRESENT = re.compile(
    r"^SWAPCHAIN_PRESENT: queue=(?P<queue>\S+) .* "
    r"generation=(?P<generation>[12]) .* "
    r"result=(?P<result>-?\d+) item_result=(?P<item_result>-?\d+)$"
)


@dataclass(frozen=True)
class AuditResult:
    run_id: str
    classification: str
    rgba_values: tuple[tuple[float, float, float, float], ...]
    generation_rgba_values: tuple[
        tuple[int, tuple[float, float, float, float]], ...
    ]
    reasons: tuple[str, ...]

    @property
    def passed(self) -> bool:
        return not self.reasons


def _rgba(value: str) -> tuple[float, float, float, float]:
    channels = tuple(float(channel) for channel in value.split(","))
    if len(channels) != 4:
        raise ValueError("RGBA must contain four channels")
    return channels  # type: ignore[return-value]


def analyze_audit(text: str, run_id: str) -> AuditResult:
    run_messages = [
        match.group(2)
        for line in text.splitlines()
        if (match := TAGGED.match(line)) and match.group(1) == run_id
    ]
    messages = [
        message
        for message in run_messages
        if message.startswith(("STARTUP_COLOR_", "SWAPCHAIN_"))
    ]
    reasons: list[str] = []
    if messages.count(
        "STARTUP_COLOR_AUDIT_BEGIN: generation_limit=2 "
        "generation_2_present_limit=180"
    ) != 1:
        reasons.append("the exact run did not arm one bounded two-generation audit")
    finish_indices = [
        index
        for index, message in enumerate(messages)
        if message.startswith(
            "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
            "generation=2 ordinal=180"
        )
    ]
    if len(finish_indices) != 1:
        reasons.append("the audit did not finish exactly once at the generation-2 bound")
    elif finish_indices[0] != len(messages) - 1:
        reasons.append(
            "the audit emitted startup/lifecycle records after its bounded finish gate"
        )

    submits: list[tuple[int, int, str, str]] = []
    presents: list[tuple[int, int, str]] = []
    clears: list[
        tuple[int, int, str, tuple[float, float, float, float]]
    ] = []
    begin_clears: list[
        tuple[int, int, str, int, tuple[float, float, float, float]]
    ] = []
    for index, message in enumerate(messages):
        if match := SUBMIT.match(message):
            if int(match.group("result")) == 0:
                submits.append(
                    (
                        index,
                        int(match.group("generation")),
                        match.group("queue"),
                        match.group("framebuffer"),
                    )
                )
        elif match := PRESENT.match(message):
            if (
                int(match.group("result")) in (VK_SUCCESS, VK_SUBOPTIMAL_KHR)
                and int(match.group("item_result"))
                in (VK_SUCCESS, VK_SUBOPTIMAL_KHR)
            ):
                presents.append(
                    (index, int(match.group("generation")), match.group("queue"))
                )
        elif match := CLEAR.match(message):
            try:
                clears.append(
                    (
                        index,
                        int(match.group("generation")),
                        match.group("framebuffer"),
                        _rgba(match.group("rgba")),
                    )
                )
            except ValueError:
                reasons.append("a vkCmdClearAttachments RGBA record was malformed")
        elif match := BEGIN_CLEAR.match(message):
            try:
                begin_clears.append(
                    (
                        index,
                        int(match.group("generation")),
                        match.group("framebuffer"),
                        int(match.group("load_op")),
                        _rgba(match.group("rgba")),
                    )
                )
            except ValueError:
                reasons.append("a begin-pass RGBA record was malformed")

    for generation in (1, 2):
        generation_submits = [item for item in submits if item[1] == generation]
        generation_presents = [item for item in presents if item[1] == generation]
        if not generation_submits:
            reasons.append(
                f"no successful generation-{generation} command-buffer submit was recorded"
            )
        if not generation_presents:
            reasons.append(f"no successful generation-{generation} present was recorded")
        if generation_submits and generation_presents and not any(
            submit_index < present_index and submit_queue == present_queue
            for submit_index, _, submit_queue, _ in generation_submits
            for present_index, _, present_queue in generation_presents
        ):
            reasons.append(
                f"no generation-{generation} submit precedes a present on the same queue"
            )

    submitted_framebuffers = {
        (generation, framebuffer)
        for _, generation, _, framebuffer in submits
    }
    linked_clears = [
        (generation, rgba)
        for _, generation, framebuffer, rgba in clears
        if (generation, framebuffer) in submitted_framebuffers
    ]
    linked_begin_clears = [
        (generation, rgba)
        for _, generation, framebuffer, load_op, rgba in begin_clears
        if (generation, framebuffer) in submitted_framebuffers and load_op == 1
    ]
    generation_rgba_values = tuple(linked_begin_clears + linked_clears)
    rgba_values = tuple(rgba for _, rgba in generation_rgba_values)
    if linked_clears:
        classification = "explicit-clear-attachments"
    elif linked_begin_clears:
        classification = "render-pass-loadop-clear"
    else:
        classification = "no-submitted-color-clear"
    return AuditResult(
        run_id,
        classification,
        rgba_values,
        generation_rgba_values,
        tuple(reasons),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--run-id", required=True)
    args = parser.parse_args()
    result = analyze_audit(args.log.read_text(errors="replace"), args.run_id)
    print(f"classification: {result.classification}")
    rgba_counts = Counter(result.generation_rgba_values)
    for (generation, rgba), count in sorted(rgba_counts.items()):
        print(
            f"generation={generation} rgba: "
            + ",".join(f"{channel:.9g}" for channel in rgba)
            + f" count={count}"
        )
    print("verdict: " + ("PASS" if result.passed else "INCONCLUSIVE"))
    for reason in result.reasons:
        print(f"reason: {reason}")
    return 0 if result.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
