#!/usr/bin/env python3
"""Recover ESO's direct GIPA/GDPA queries and compare probe results."""

from __future__ import annotations

import argparse
import struct
from collections import defaultdict
from pathlib import Path

from analyze_vk_calls import find_section, image_base, integer, load_sections


def read_cstring(data: bytes, offset: int, limit: int = 128) -> str | None:
    if not 0 <= offset < len(data):
        return None
    end = data.find(b"\0", offset, min(len(data), offset + limit))
    if end < 0:
        return None
    try:
        value = data[offset:end].decode("ascii")
    except UnicodeDecodeError:
        return None
    return value if value.startswith("vk") and value[2:3].isupper() else None


def nearest_proc_name(
    data: bytes, code: bytes, code_offset: int, text_address: int, base: int
) -> str | None:
    candidates = []
    for offset in range(max(0, code_offset - 256), code_offset - 6):
        if (
            0x48 <= code[offset] <= 0x4F
            and code[offset] & 0x08
            and code[offset + 1] == 0x8D
            and code[offset + 2] & 0xC7 == 0x05
        ):
            source = text_address + offset
            displacement = struct.unpack_from("<i", code, offset + 3)[0]
            target = source + 7 + displacement
            name = read_cstring(data, target - base)
            if name:
                candidates.append((offset, name))
    return candidates[-1][1] if candidates else None


def find_queries(
    executable_path: Path, slots: dict[int, str]
) -> tuple[dict[str, list[tuple[int, str]]], list[tuple[int, str]]]:
    data = executable_path.read_bytes()
    sections = load_sections(executable_path)
    text = find_section(sections, "__TEXT", "__text")
    base = image_base(sections)
    absolute_slots = {base + offset: route for offset, route in slots.items()}
    code = data[text.offset : text.offset + text.size]
    queries: dict[str, list[tuple[int, str]]] = defaultdict(list)
    unknown = []

    for offset in range(len(code) - 6):
        if code[offset : offset + 2] != b"\xff\x15":
            continue
        source = text.address + offset
        displacement = struct.unpack_from("<i", code, offset + 2)[0]
        route = absolute_slots.get(source + 6 + displacement)
        if not route:
            continue
        name = nearest_proc_name(data, code, offset, text.address, base)
        if name:
            queries[route].append((source, name))
        else:
            unknown.append((source, route))
    return queries, unknown


def load_probe(path: Path) -> dict[str, dict[str, str]]:
    results: dict[str, dict[str, str]] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("proc vk"):
            continue
        fields = line.split()
        results[fields[1]] = {
            "GIPA": fields[2].split("=", 1)[1],
            "GDPA": fields[3].split("=", 1)[1],
        }
    return results


def compare_probes(
    queries: dict[str, list[tuple[int, str]]], old_path: Path, new_path: Path
) -> bool:
    old = load_probe(old_path)
    new = load_probe(new_path)
    regressions = []
    missing = []
    for route, items in queries.items():
        for name in sorted({name for _, name in items}):
            if name not in old or name not in new:
                missing.append((route, name))
            elif old[name][route] == "yes" and new[name][route] == "NULL":
                regressions.append((route, name))

    print(f"probe candidates: old={len(old)} new={len(new)}")
    print(f"queried names absent from probe output: {len(missing)}")
    for route, name in missing:
        print(f"  {route} {name}")
    print(f"old-nonnull/new-null on ESO query route: {len(regressions)}")
    for route, name in regressions:
        print(f"  {route} {name}")
    return not missing and not regressions


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--gipa-slot", required=True, type=integer)
    parser.add_argument("--gdpa-slot", required=True, type=integer)
    parser.add_argument("--old-probe-output", type=Path)
    parser.add_argument("--new-probe-output", type=Path)
    args = parser.parse_args()
    if bool(args.old_probe_output) != bool(args.new_probe_output):
        parser.error("provide both --old-probe-output and --new-probe-output")

    queries, unknown = find_queries(
        args.exe, {args.gipa_slot: "GIPA", args.gdpa_slot: "GDPA"}
    )
    for route in ("GIPA", "GDPA"):
        items = queries.get(route, [])
        names = sorted({name for _, name in items})
        print(f"{route} direct query sites: {len(items)}; unique names: {len(names)}")
        for name in names:
            sources = ", ".join(
                f"0x{source:x}" for source, item_name in items if item_name == name
            )
            print(f"  {name:48s} {sources}")
    print(f"query sites without a recovered name: {len(unknown)}")
    for source, route in unknown:
        print(f"  0x{source:x} {route}")

    compatible = True
    if args.old_probe_output:
        compatible = compare_probes(
            queries, args.old_probe_output, args.new_probe_output
        )
    if unknown or not compatible:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
