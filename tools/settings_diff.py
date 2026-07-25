#!/usr/bin/env python3
"""Compare ESO UserSettings files after applying declared expected values."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re


SETTING = re.compile(r'^SET (?P<key>\S+) "(?P<value>.*)"(?P<ending>\r?\n)?$')


def sha256_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def parse_expected(values: list[str]) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            raise ValueError(f"expected KEY=VALUE, got: {value}")
        key, setting = value.split("=", 1)
        if not key or key in parsed:
            raise ValueError(f"invalid or duplicate expected setting: {key}")
        parsed[key] = setting
    return parsed


def normalize(
    text: str,
    expected: dict[str, str],
) -> str:
    lines = text.splitlines(keepends=True)
    counts = {key: 0 for key in expected}
    rendered: list[str] = []
    for line in lines:
        match = SETTING.fullmatch(line)
        if match is None or match.group("key") not in expected:
            rendered.append(line)
            continue
        key = match.group("key")
        counts[key] += 1
        ending = match.group("ending") or ""
        rendered.append(f'SET {key} "{expected[key]}"{ending}')
    invalid = [key for key, count in counts.items() if count != 1]
    if invalid:
        raise ValueError(
            "expected exactly one line for: " + ", ".join(sorted(invalid))
        )
    return "".join(rendered)


def setting_entries(text: str) -> tuple[list[tuple[str, str]], list[str]]:
    settings: list[tuple[str, str]] = []
    structure: list[str] = []
    for line in text.splitlines(keepends=True):
        match = SETTING.fullmatch(line)
        if match is None:
            structure.append(line)
        else:
            key = match.group("key")
            settings.append((key, match.group("value")))
            structure.append(f"SET {key}\n")
    keys = [key for key, _ in settings]
    if len(keys) != len(set(keys)):
        raise ValueError("duplicate setting keys prevent exact comparison")
    return settings, structure


def compare(
    reference: str,
    actual: str,
    expected: dict[str, str],
) -> tuple[str, list[tuple[str, str, str]]]:
    normalized = normalize(reference, expected)
    before, before_structure = setting_entries(normalized)
    after, after_structure = setting_entries(actual)
    if before_structure != after_structure:
        raise ValueError("settings file structure changed")
    before_map = dict(before)
    after_map = dict(after)
    if before_map.keys() != after_map.keys():
        raise ValueError("settings key set changed")
    changes = [
        (key, before_map[key], after_map[key])
        for key in before_map
        if before_map[key] != after_map[key]
    ]
    return normalized, changes


def command_compare(args: argparse.Namespace) -> int:
    reference = args.reference.read_bytes().decode("utf-8")
    actual = args.actual.read_bytes().decode("utf-8")
    expected = parse_expected(args.expected)
    normalized, changes = compare(reference, actual, expected)
    if args.normalized_output is not None:
        if args.normalized_output.exists():
            raise ValueError(
                f"refusing to overwrite normalized output: {args.normalized_output}"
            )
        args.normalized_output.write_bytes(normalized.encode("utf-8"))
    print(f"Normalized reference SHA-256: {sha256_text(normalized)}")
    print(f"Actual SHA-256: {sha256_text(actual)}")
    print("Structural identity: exact")
    print(f"Changed settings: {len(changes)}")
    for key, before, after in changes:
        print(f'SET {key}: "{before}" -> "{after}"')
    return 0


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("reference", type=Path)
    value.add_argument("actual", type=Path)
    value.add_argument("--expected", action="append", default=[])
    value.add_argument("--normalized-output", type=Path)
    value.set_defaults(function=command_compare)
    return value


def main() -> int:
    args = parser().parse_args()
    try:
        return args.function(args)
    except (OSError, UnicodeError, ValueError) as error:
        print(f"ERROR: {error}")
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
