#!/usr/bin/env python3
"""Generate the relocation-tolerant public-installer compatibility profile."""

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from analyze_vk_calls import find_section, image_base, load_sections
from eso_update import archive_member


KIND_VALUES = {"call": 1, "jump": 2, "address": 3, "immediate": 4, "pointer": 5}
ROUTE_VALUES = {"GIPA": 1, "GDPA": 2}


def c_string(value: str) -> str:
    return json.dumps(value)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    profile = json.loads(args.manifest.read_text(encoding="utf-8"))
    legacy = profile["analysis"]["legacy_moltenvk"]
    object_bytes = archive_member(args.archive, legacy["archive_member"])
    with tempfile.TemporaryDirectory(prefix="eso-compat-profile-") as directory:
        object_path = Path(directory) / "MoltenVK.o"
        object_path.write_bytes(object_bytes)
        object_text = find_section(load_sections(object_path), None, "__text")

    link_delta = int(legacy["link_delta"], 0)
    executable = args.exe.read_bytes()
    executable_sections = load_sections(args.exe)
    text = find_section(executable_sections, "__TEXT", "__text")
    base = image_base(executable_sections)
    old_start = base + link_delta + object_text.address
    old_end = old_start + object_text.size
    code = executable[text.offset : text.offset + text.size]
    reference_counts: Counter[tuple[int, str]] = Counter()

    def add_reference(source: int, target: int, kind: str) -> None:
        if old_start <= target < old_end and not old_start <= source < old_end:
            reference_counts[(target - base, kind)] += 1

    for offset in range(len(code)):
        source = text.address + offset
        if offset + 5 <= len(code) and code[offset] in (0xE8, 0xE9):
            displacement = struct.unpack_from("<i", code, offset + 1)[0]
            add_reference(
                source,
                source + 5 + displacement,
                "call" if code[offset] == 0xE8 else "jump",
            )
        if (
            offset + 7 <= len(code)
            and 0x48 <= code[offset] <= 0x4F
            and code[offset] & 0x08
            and code[offset + 1] == 0x8D
            and code[offset + 2] & 0xC7 == 0x05
        ):
            displacement = struct.unpack_from("<i", code, offset + 3)[0]
            add_reference(source, source + 7 + displacement, "address")
        if (
            offset + 10 <= len(code)
            and 0x48 <= code[offset] <= 0x4F
            and code[offset] & 0x08
            and 0xB8 <= code[offset + 1] <= 0xBF
        ):
            add_reference(
                source,
                struct.unpack_from("<Q", code, offset + 2)[0],
                "immediate",
            )

    fixups = subprocess.check_output(
        ["xcrun", "dyld_info", "-fixups", str(args.exe)], text=True
    )
    fixup_pattern = re.compile(
        r"^\s*__\w+\s+__\w+\s+0x([0-9A-Fa-f]+)\s+rebase\s+0x([0-9A-Fa-f]+)"
    )
    for line in fixups.splitlines():
        match = fixup_pattern.match(line)
        if match:
            add_reference(
                int(match.group(1), 16), int(match.group(2), 16), "pointer"
            )

    references = [
        (target, KIND_VALUES[kind], count)
        for (target, kind), count in sorted(reference_counts.items())
    ]

    queries = []
    for route, details in sorted(profile["analysis"]["proc_queries"]["routes"].items()):
        if route not in ROUTE_VALUES:
            raise SystemExit(f"unsupported proc route: {route}")
        counts = Counter(item.split(":", 1)[1] for item in details["queries"])
        for name, count in sorted(counts.items()):
            queries.append((ROUTE_VALUES[route], name, count))

    lines = [
        "/* Generated compatibility fingerprint. Do not edit. */",
        "#pragma once",
        "",
        f"#define COMPAT_OLD_TEXT_START 0x{link_delta + object_text.address:x}ULL",
        f"#define COMPAT_OLD_TEXT_END 0x{link_delta + object_text.address + object_text.size:x}ULL",
        f"#define COMPAT_GIPA_SLOT {profile['proc_addr_slots']['vkGetInstanceProcAddr']}ULL",
        f"#define COMPAT_GDPA_SLOT {profile['proc_addr_slots']['vkGetDeviceProcAddr']}ULL",
        "",
        f"static const CompatPatch kCompatPatches[{len(profile['targets'])}] = {{",
    ]
    for target in profile["targets"]:
        expected = bytes.fromhex(target["expected_bytes"])
        byte_list = ", ".join(f"0x{byte:02x}" for byte in expected)
        lines.append(
            f"    {{0x{int(target['image_offset'], 0):x}ULL, {{{byte_list}}}}},"
        )
    lines.extend(("};", f"#define COMPAT_PATCH_COUNT {len(profile['targets'])}", ""))

    lines.append(
        f"static const CompatReference kCompatReferences[{len(references)}] = {{"
    )
    for target, kind, count in references:
        lines.append(f"    {{0x{target:x}ULL, {kind}, {count}}},")
    lines.extend(
        ("};", f"#define COMPAT_REFERENCE_COUNT {len(references)}", "")
    )

    lines.append(f"static const CompatQuery kCompatQueries[{len(queries)}] = {{")
    for route, name, count in queries:
        lines.append(f"    {{{route}, {c_string(name)}, {count}}},")
    lines.extend(("};", f"#define COMPAT_QUERY_COUNT {len(queries)}", ""))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")


if __name__ == "__main__":
    main()
