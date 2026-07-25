#!/usr/bin/env python3
"""Summarize macOS WindowServer focus evidence for one ESO process."""

from __future__ import annotations

import argparse
from pathlib import Path
import re


TIMESTAMP = re.compile(r"^(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})")


def summarize(text: str, pid: int) -> dict[str, object]:
    pid_text = str(pid)
    lines = text.splitlines()
    frontmost = [
        line
        for line in lines
        if pid_text in line
        and (
            "Process now frontmost:" in line
            or "Deferring events from frontmost process" in line
        )
    ]
    keyboard = [
        line
        for line in lines
        if pid_text in line
        and (
            "keyboardFocus" in line
            or "destination <keyboardFocus" in line
        )
    ]
    denied = [
        line
        for line in lines
        if pid_text in line and "Denying the request" in line
    ]
    deaths = [
        line
        for line in lines
        if pid_text in line and "Process death:" in line
    ]
    timestamps = [
        match.group("timestamp")
        for line in frontmost + keyboard
        if (match := TIMESTAMP.match(line))
    ]
    return {
        "frontmost_confirmations": len(frontmost),
        "keyboard_focus_records": len(keyboard),
        "startup_denials": len(denied),
        "process_deaths": len(deaths),
        "first_focus_record": min(timestamps) if timestamps else "missing",
        "last_focus_record": max(timestamps) if timestamps else "missing",
    }


def command_analyze(args: argparse.Namespace) -> int:
    text = args.log.read_text(encoding="utf-8", errors="replace")
    result = summarize(text, args.pid)
    print(f"focus pid: {args.pid}")
    for key, value in result.items():
        print(f"{key.replace('_', ' ')}: {value}")
    foreground = (
        result["frontmost_confirmations"] > 0
        and result["keyboard_focus_records"] > 0
    )
    print(
        "OS focus evidence: "
        + ("FOREGROUND" if foreground else "INDETERMINATE")
    )
    return 0 if foreground else 2


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("log", type=Path)
    value.add_argument("--pid", type=int, required=True)
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
