#!/usr/bin/env python3
"""Find references from ESO to a statically linked MoltenVK object.

The analysis covers direct rel32 calls/jumps, RIP-relative address-taking LEA
instructions, absolute mov-immediates, and rebased pointers reported by dyld.
It intentionally reports references from outside the linked MoltenVK text
range; references internal to the old runtime do not cross an ownership
boundary.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import subprocess
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


MACH_HEADER_64 = struct.Struct("<IiiIIIII")
LOAD_COMMAND = struct.Struct("<II")
SEGMENT_COMMAND_64 = struct.Struct("<II16sQQQQiiII")
SECTION_64 = struct.Struct("<16s16sQQIIIIIIII")
MH_MAGIC_64 = 0xFEEDFACF
LC_SEGMENT_64 = 0x19


@dataclass(frozen=True)
class Section:
    segment: str
    name: str
    address: int
    size: int
    offset: int


@dataclass(frozen=True)
class Reference:
    source: int
    kind: str


def integer(value: str) -> int:
    return int(value, 0)


def cstring(value: bytes) -> str:
    return value.split(b"\0", 1)[0].decode("ascii")


def load_sections(path: Path) -> list[Section]:
    data = path.read_bytes()
    if len(data) < MACH_HEADER_64.size:
        raise ValueError(f"{path}: file is too small for a Mach-O header")
    magic, _, _, _, command_count, _, _, _ = MACH_HEADER_64.unpack_from(data)
    if magic != MH_MAGIC_64:
        raise ValueError(f"{path}: expected a little-endian 64-bit Mach-O")

    sections = []
    cursor = MACH_HEADER_64.size
    for _ in range(command_count):
        command, command_size = LOAD_COMMAND.unpack_from(data, cursor)
        if command_size < LOAD_COMMAND.size or cursor + command_size > len(data):
            raise ValueError(f"{path}: invalid Mach-O load command")
        if command == LC_SEGMENT_64:
            values = SEGMENT_COMMAND_64.unpack_from(data, cursor)
            segment_name = cstring(values[2])
            section_count = values[9]
            section_cursor = cursor + SEGMENT_COMMAND_64.size
            for _ in range(section_count):
                if section_cursor + SECTION_64.size > cursor + command_size:
                    raise ValueError(f"{path}: invalid Mach-O section table")
                section = SECTION_64.unpack_from(data, section_cursor)
                sections.append(
                    Section(
                        segment=cstring(section[1]) or segment_name,
                        name=cstring(section[0]),
                        address=section[2],
                        size=section[3],
                        offset=section[4],
                    )
                )
                section_cursor += SECTION_64.size
        cursor += command_size
    return sections


def find_section(sections: list[Section], segment: str | None, name: str) -> Section:
    matches = [
        section
        for section in sections
        if section.name == name and (segment is None or section.segment == segment)
    ]
    if len(matches) != 1:
        raise ValueError(f"expected one {segment or '*'}, {name} section; found {len(matches)}")
    return matches[0]


def image_base(sections: list[Section]) -> int:
    candidates = [section.address - section.offset for section in sections if section.offset]
    if not candidates:
        raise ValueError("could not determine Mach-O image base")
    return min(candidates)


def load_symbols(obj: Path, link_delta: int, base: int) -> dict[int, str]:
    output = subprocess.check_output(["nm", "-n", str(obj)], text=True)
    symbols = {}
    for line in output.splitlines():
        match = re.match(r"^([0-9a-f]+) [Tt] (_vk\w+)$", line)
        if match:
            symbols[base + link_delta + int(match.group(1), 16)] = match.group(2)[1:]
    if not symbols:
        raise ValueError(f"{obj}: no Vulkan text symbols found")
    return symbols


def load_exports(runtime: Path) -> set[str]:
    output = subprocess.check_output(
        ["nm", "-arch", "x86_64", "-gU", str(runtime)], text=True
    )
    return {
        match.group(1)
        for line in output.splitlines()
        if (match := re.search(r" _?(vk\w+)$", line))
    }


def add_reference(
    references: dict[str, set[Reference]],
    symbols: dict[int, str],
    source: int,
    target: int,
    kind: str,
    old_text_start: int,
    old_text_end: int,
) -> None:
    symbol = symbols.get(target)
    if symbol and not old_text_start <= source < old_text_end:
        references[symbol].add(Reference(source, kind))


def scan_text(
    executable: bytes,
    text: Section,
    symbols: dict[int, str],
    references: dict[str, set[Reference]],
    old_text_start: int,
    old_text_end: int,
) -> None:
    code = executable[text.offset : text.offset + text.size]
    for offset in range(len(code)):
        source = text.address + offset

        # Direct near call or jump with a signed rel32 displacement.
        if offset + 5 <= len(code) and code[offset] in (0xE8, 0xE9):
            displacement = struct.unpack_from("<i", code, offset + 1)[0]
            add_reference(
                references,
                symbols,
                source,
                source + 5 + displacement,
                "call" if code[offset] == 0xE8 else "jump",
                old_text_start,
                old_text_end,
            )

        # 64-bit LEA reg, disp32(%rip), used by the linker for address-taking.
        if (
            offset + 7 <= len(code)
            and 0x48 <= code[offset] <= 0x4F
            and code[offset] & 0x08
            and code[offset + 1] == 0x8D
            and code[offset + 2] & 0xC7 == 0x05
        ):
            displacement = struct.unpack_from("<i", code, offset + 3)[0]
            add_reference(
                references,
                symbols,
                source,
                source + 7 + displacement,
                "address",
                old_text_start,
                old_text_end,
            )

        # 64-bit MOV reg, imm64. Unusual in PIE code, but cheap to check.
        if (
            offset + 10 <= len(code)
            and 0x48 <= code[offset] <= 0x4F
            and code[offset] & 0x08
            and 0xB8 <= code[offset + 1] <= 0xBF
        ):
            target = struct.unpack_from("<Q", code, offset + 2)[0]
            add_reference(
                references,
                symbols,
                source,
                target,
                "immediate",
                old_text_start,
                old_text_end,
            )


def scan_dyld_fixups(
    executable_path: Path,
    symbols: dict[int, str],
    references: dict[str, set[Reference]],
    old_text_start: int,
    old_text_end: int,
) -> None:
    output = subprocess.check_output(
        ["xcrun", "dyld_info", "-fixups", str(executable_path)], text=True
    )
    pattern = re.compile(
        r"^\s*__\w+\s+__\w+\s+0x([0-9A-Fa-f]+)\s+rebase\s+0x([0-9A-Fa-f]+)"
    )
    for line in output.splitlines():
        match = pattern.match(line)
        if match:
            add_reference(
                references,
                symbols,
                int(match.group(1), 16),
                int(match.group(2), 16),
                "pointer",
                old_text_start,
                old_text_end,
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--object", required=True, type=Path)
    parser.add_argument("--link-delta", required=True, type=integer)
    parser.add_argument("--new-runtime", type=Path)
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()

    executable_sections = load_sections(args.exe)
    object_sections = load_sections(args.object)
    text = find_section(executable_sections, "__TEXT", "__text")
    object_text = find_section(object_sections, None, "__text")
    base = image_base(executable_sections)
    symbols = load_symbols(args.object, args.link_delta, base)
    old_text_start = base + args.link_delta + object_text.address
    old_text_end = old_text_start + object_text.size
    references: dict[str, set[Reference]] = defaultdict(set)

    scan_text(
        args.exe.read_bytes(),
        text,
        symbols,
        references,
        old_text_start,
        old_text_end,
    )
    scan_dyld_fixups(args.exe, symbols, references, old_text_start, old_text_end)

    total = sum(len(items) for items in references.values())
    print(f"old Vulkan text symbols: {len(symbols)}")
    print(f"old MoltenVK text range: 0x{old_text_start:x}-0x{old_text_end:x}")
    print(f"external references: {total}")
    print(f"externally referenced Vulkan entry points: {len(references)}")
    for symbol, items in sorted(
        references.items(), key=lambda item: (-len(item[1]), item[0])
    ):
        rendered = ", ".join(
            f"0x{item.source:x}:{item.kind}"
            for item in sorted(items, key=lambda item: (item.source, item.kind))
        )
        print(f"{len(items):4d} {symbol:48s} {rendered}")

    coverage_ok = True
    if args.manifest:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        manifest_symbols = {target["symbol"] for target in manifest["targets"]}
        referenced_symbols = set(references)
        missing_from_manifest = sorted(referenced_symbols - manifest_symbols)
        unreferenced_manifest = sorted(manifest_symbols - referenced_symbols)
        print(f"referenced entry points absent from manifest: {len(missing_from_manifest)}")
        for symbol in missing_from_manifest:
            print(f"  {symbol}")
        print(f"manifest entry points without an external reference: {len(unreferenced_manifest)}")
        for symbol in unreferenced_manifest:
            print(f"  {symbol}")
        coverage_ok = not missing_from_manifest and not unreferenced_manifest

    if args.new_runtime:
        exports = load_exports(args.new_runtime)
        old_names = set(symbols.values())
        unavailable = sorted(old_names - exports)
        referenced_unavailable = sorted(set(references) - exports)
        print(f"new runtime Vulkan exports: {len(exports)}")
        print(f"old entry points unavailable in new runtime: {len(unavailable)}")
        for symbol in unavailable:
            print(f"  {symbol}")
        print(
            "externally referenced entry points unavailable in new runtime: "
            f"{len(referenced_unavailable)}"
        )
        for symbol in referenced_unavailable:
            print(f"  {symbol}")
        if referenced_unavailable:
            coverage_ok = False

    if not coverage_ok:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
