#!/usr/bin/env python3
"""Find references from ESO to a statically linked MoltenVK object.

The analysis covers direct rel32 calls/jumps, RIP-relative address-taking LEA
instructions, absolute mov-immediates, and rebased pointers from chained fixups.
It intentionally reports references from outside the linked MoltenVK text
range; references internal to the old runtime do not cross an ownership
boundary.
"""

from __future__ import annotations

import argparse
import bisect
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
LC_DYLD_CHAINED_FIXUPS = 0x80000034
LC_DYLD_INFO = 0x22
LC_DYLD_INFO_ONLY = 0x80000022
LC_FUNCTION_STARTS = 0x26
REBASE_TYPE_POINTER = 1
REBASE_OPCODE_MASK = 0xF0
REBASE_IMMEDIATE_MASK = 0x0F
REBASE_OPCODE_DONE = 0x00
REBASE_OPCODE_SET_TYPE_IMM = 0x10
REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB = 0x20
REBASE_OPCODE_ADD_ADDR_ULEB = 0x30
REBASE_OPCODE_ADD_ADDR_IMM_SCALED = 0x40
REBASE_OPCODE_DO_REBASE_IMM_TIMES = 0x50
REBASE_OPCODE_DO_REBASE_ULEB_TIMES = 0x60
REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB = 0x70
REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB = 0x80
DYLD_CHAINED_PTR_64 = 2
DYLD_CHAINED_PTR_64_OFFSET = 6
DYLD_CHAINED_PTR_START_NONE = 0xFFFF
DYLD_CHAINED_PTR_START_MULTI = 0x8000
DYLD_CHAINED_PTR_START_LAST = 0x8000


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


def fixup_rebases(path: Path) -> list[tuple[int, int]]:
    """Return (source, target) pointer rebases without invoking dyld_info.

    Earlier selected ESO builds used LC_DYLD_CHAINED_FIXUPS. ESO 12.1.5 uses
    classic LC_DYLD_INFO_ONLY rebase opcodes, and Xcode 27's
    `dyld_info -fixups` traps while symbolizing one of its bind targets. Both
    encodings are decoded directly. An image with neither or both encodings,
    an unsupported pointer format, or a malformed table raises.
    """
    data = path.read_bytes()
    magic, _, _, _, command_count, _, _, _ = MACH_HEADER_64.unpack_from(data)
    if magic != MH_MAGIC_64:
        raise ValueError(f"{path}: expected a little-endian 64-bit Mach-O")
    segments: list[tuple[int, int, int]] = []
    chained: tuple[int, int] | None = None
    rebase_info: tuple[int, int] | None = None
    cursor = MACH_HEADER_64.size
    for _ in range(command_count):
        command, command_size = LOAD_COMMAND.unpack_from(data, cursor)
        if command_size < LOAD_COMMAND.size or cursor + command_size > len(data):
            raise ValueError(f"{path}: invalid Mach-O load command")
        if command == LC_SEGMENT_64:
            values = SEGMENT_COMMAND_64.unpack_from(data, cursor)
            segments.append((values[3], values[5], values[6]))
        elif command == LC_DYLD_CHAINED_FIXUPS:
            chained = struct.unpack_from("<II", data, cursor + 8)
        elif command in (LC_DYLD_INFO, LC_DYLD_INFO_ONLY):
            rebase_info = struct.unpack_from("<II", data, cursor + 8)
            if rebase_info[1] == 0:
                rebase_info = None
        cursor += command_size
    if (chained is None) == (rebase_info is None):
        raise ValueError(f"{path}: expected exactly one fixup encoding")
    if chained is not None:
        return _chained_rebases(path, data, segments, chained)
    assert rebase_info is not None
    return _opcode_rebases(path, data, segments, rebase_info)


def _chained_rebases(
    path: Path,
    data: bytes,
    segments: list[tuple[int, int, int]],
    fixups: tuple[int, int],
) -> list[tuple[int, int]]:
    base = min(vmaddr for vmaddr, fileoff, filesize in segments if fileoff == 0 and filesize)
    payload_offset, payload_size = fixups
    if payload_offset + payload_size > len(data) or payload_size < 28:
        raise ValueError(f"{path}: invalid chained fixup payload")
    payload = data[payload_offset : payload_offset + payload_size]
    starts_offset = struct.unpack_from("<I", payload, 4)[0]
    (segment_count,) = struct.unpack_from("<I", payload, starts_offset)
    if segment_count > len(segments):
        raise ValueError(f"{path}: chained fixup segment count exceeds segments")
    info_offsets = struct.unpack_from(f"<{segment_count}I", payload, starts_offset + 4)

    rebases: list[tuple[int, int]] = []

    def walk(segment: tuple[int, int, int], pointer_format: int, offset: int) -> None:
        vmaddr, fileoff, filesize = segment
        while True:
            if offset + 8 > filesize:
                raise ValueError(f"{path}: chained fixup outside segment")
            (raw,) = struct.unpack_from("<Q", data, fileoff + offset)
            if not raw >> 63:
                target = raw & ((1 << 36) - 1)
                if pointer_format == DYLD_CHAINED_PTR_64_OFFSET:
                    target += base
                else:
                    target |= ((raw >> 36) & 0xFF) << 56
                rebases.append((vmaddr + offset, target))
            step = (raw >> 51) & 0xFFF
            if step == 0:
                return
            offset += step * 4

    for index, info_offset in enumerate(info_offsets):
        if info_offset == 0:
            continue
        info = starts_offset + info_offset
        size, page_size, pointer_format, _, _, page_count = struct.unpack_from(
            "<IHHQIH", payload, info
        )
        if pointer_format not in (DYLD_CHAINED_PTR_64, DYLD_CHAINED_PTR_64_OFFSET):
            raise ValueError(f"{path}: unsupported chained pointer format {pointer_format}")
        if page_size == 0 or info + size > payload_size:
            raise ValueError(f"{path}: invalid chained starts for segment {index}")
        page_starts = struct.unpack_from(f"<{page_count}H", payload, info + 22)
        overflow = info + 22 + 2 * page_count
        for page, start in enumerate(page_starts):
            if start == DYLD_CHAINED_PTR_START_NONE:
                continue
            if not start & DYLD_CHAINED_PTR_START_MULTI:
                walk(segments[index], pointer_format, page * page_size + start)
                continue
            entry = overflow + 2 * (start & ~DYLD_CHAINED_PTR_START_MULTI)
            while True:
                if entry + 2 > info + size:
                    raise ValueError(f"{path}: invalid chained start overflow")
                (chain_start,) = struct.unpack_from("<H", payload, entry)
                walk(
                    segments[index],
                    pointer_format,
                    page * page_size + (chain_start & ~DYLD_CHAINED_PTR_START_LAST),
                )
                if chain_start & DYLD_CHAINED_PTR_START_LAST:
                    break
                entry += 2
    return rebases


def _opcode_rebases(
    path: Path,
    data: bytes,
    segments: list[tuple[int, int, int]],
    rebase_info: tuple[int, int],
) -> list[tuple[int, int]]:
    offset, size = rebase_info
    if offset + size > len(data):
        raise ValueError(f"{path}: invalid rebase opcode payload")
    stream = data[offset : offset + size]
    cursor = 0
    segment: tuple[int, int, int] | None = None
    segment_offset = 0
    rebases: list[tuple[int, int]] = []

    def uleb() -> int:
        nonlocal cursor
        value = shift = 0
        while True:
            if cursor >= len(stream):
                raise ValueError(f"{path}: truncated rebase ULEB128")
            byte = stream[cursor]
            cursor += 1
            value |= (byte & 0x7F) << shift
            shift += 7
            if not byte & 0x80:
                return value

    def rebase(skip: int = 0) -> None:
        nonlocal segment_offset
        if segment is None:
            raise ValueError(f"{path}: rebase before segment selection")
        vmaddr, fileoff, filesize = segment
        if segment_offset + 8 > filesize:
            raise ValueError(f"{path}: rebase outside segment")
        (target,) = struct.unpack_from("<Q", data, fileoff + segment_offset)
        rebases.append((vmaddr + segment_offset, target))
        segment_offset += 8 + skip

    while cursor < len(stream):
        byte = stream[cursor]
        cursor += 1
        opcode, immediate = byte & REBASE_OPCODE_MASK, byte & REBASE_IMMEDIATE_MASK
        if opcode == REBASE_OPCODE_DONE:
            break
        if opcode == REBASE_OPCODE_SET_TYPE_IMM:
            if immediate != REBASE_TYPE_POINTER:
                raise ValueError(f"{path}: unsupported rebase type {immediate}")
        elif opcode == REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
            if immediate >= len(segments):
                raise ValueError(f"{path}: rebase segment {immediate} is absent")
            segment = segments[immediate]
            segment_offset = uleb()
        elif opcode == REBASE_OPCODE_ADD_ADDR_ULEB:
            segment_offset = (segment_offset + uleb()) & ((1 << 64) - 1)
        elif opcode == REBASE_OPCODE_ADD_ADDR_IMM_SCALED:
            segment_offset += immediate * 8
        elif opcode == REBASE_OPCODE_DO_REBASE_IMM_TIMES:
            for _ in range(immediate):
                rebase()
        elif opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES:
            for _ in range(uleb()):
                rebase()
        elif opcode == REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB:
            rebase(uleb())
        elif opcode == REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB:
            count, skip = uleb(), uleb()
            for _ in range(count):
                rebase(skip)
        else:
            raise ValueError(f"{path}: unknown rebase opcode 0x{byte:02x}")
    return rebases


def function_starts(path: Path) -> list[int]:
    """Return sorted function start addresses from LC_FUNCTION_STARTS."""
    data = path.read_bytes()
    magic, _, _, _, command_count, _, _, _ = MACH_HEADER_64.unpack_from(data)
    if magic != MH_MAGIC_64:
        raise ValueError(f"{path}: expected a little-endian 64-bit Mach-O")
    text_address: int | None = None
    table: tuple[int, int] | None = None
    cursor = MACH_HEADER_64.size
    for _ in range(command_count):
        command, command_size = LOAD_COMMAND.unpack_from(data, cursor)
        if command == LC_SEGMENT_64:
            values = SEGMENT_COMMAND_64.unpack_from(data, cursor)
            if cstring(values[2]) == "__TEXT":
                text_address = values[3]
        elif command == LC_FUNCTION_STARTS:
            table = struct.unpack_from("<II", data, cursor + 8)
        cursor += command_size
    if text_address is None or table is None:
        raise ValueError(f"{path}: no __TEXT segment or LC_FUNCTION_STARTS table")
    offset, size = table
    stream = data[offset : offset + size]
    starts = []
    address = text_address
    cursor = 0
    while cursor < len(stream):
        value = shift = 0
        while True:
            byte = stream[cursor]
            cursor += 1
            value |= (byte & 0x7F) << shift
            shift += 7
            if not byte & 0x80:
                break
        if value == 0:
            break
        address += value
        starts.append(address)
    return starts


def unaligned_sources(path: Path, sources: set[int]) -> set[int]:
    """Return sources proven to lie inside, not at the start of, an instruction.

    The byte scanners intentionally over-approximate. A candidate is rejected
    only when llvm-objdump, decoding from the enclosing LC_FUNCTION_STARTS
    entry, places an instruction boundary on each side of it. Any site that
    cannot be decoded that way is kept.
    """
    starts = function_starts(path)
    rejected = set()
    for source in sorted(sources):
        index = bisect.bisect_right(starts, source) - 1
        if index < 0:
            continue
        output = subprocess.run(
            [
                "xcrun",
                "llvm-objdump",
                "-d",
                "--no-show-raw-insn",
                f"--start-address=0x{starts[index]:x}",
                f"--stop-address=0x{source + 16:x}",
                str(path),
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        if output.returncode:
            continue
        boundaries = {
            int(match.group(1), 16)
            for line in output.stdout.splitlines()
            if (match := re.match(r"^\s*([0-9a-f]+):\s", line))
        }
        if source in boundaries:
            continue
        if any(address < source for address in boundaries) and any(
            address > source for address in boundaries
        ):
            rejected.add(source)
    return rejected


def scan_dyld_fixups(
    executable_path: Path,
    symbols: dict[int, str],
    references: dict[str, set[Reference]],
    old_text_start: int,
    old_text_end: int,
) -> None:
    for source, target in fixup_rebases(executable_path):
        add_reference(
            references,
            symbols,
            source,
            target,
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
