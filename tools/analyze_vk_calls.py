#!/usr/bin/env python3
"""Find x86 rel32 calls/jumps from ESO into a local static MoltenVK object."""

from __future__ import annotations

import argparse
import re
import struct
import subprocess
from collections import defaultdict
from pathlib import Path


def load_symbols(obj: Path, link_delta: int) -> dict[int, str]:
    output = subprocess.check_output(["nm", "-n", str(obj)], text=True)
    symbols = {}
    for line in output.splitlines():
        match = re.match(r"^([0-9a-f]+) [Tt] (_vk\w+)$", line)
        if match:
            symbols[link_delta + int(match.group(1), 16)] = match.group(2)[1:]
    return symbols


def integer(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--object", required=True, type=Path)
    parser.add_argument("--link-delta", required=True, type=integer)
    parser.add_argument("--text-file-offset", required=True, type=integer)
    parser.add_argument("--text-vmaddr", required=True, type=integer)
    parser.add_argument("--text-size", required=True, type=integer)
    parser.add_argument("--mvk-text-size", required=True, type=integer)
    args = parser.parse_args()

    data = args.exe.read_bytes()
    symbols = load_symbols(args.object, args.link_delta)
    calls = defaultdict(list)
    start = args.text_file_offset
    end = start + args.text_size
    mvk_end = args.link_delta + args.mvk_text_size

    for offset in range(start, end - 5):
        if data[offset] not in (0xE8, 0xE9):
            continue
        displacement = struct.unpack_from("<i", data, offset + 1)[0]
        caller = args.text_vmaddr + offset - args.text_file_offset
        target = caller + 5 + displacement
        symbol = symbols.get(target)
        if symbol and not (args.link_delta <= caller < mvk_end):
            calls[symbol].append((caller, "call" if data[offset] == 0xE8 else "jump"))

    print(f"direct external calls: {sum(map(len, calls.values()))}")
    print(f"Vulkan entry points called directly: {len(calls)}")
    for symbol, items in sorted(calls.items(), key=lambda item: (-len(item[1]), item[0])):
        references = ", ".join(f"0x{address:x}:{kind}" for address, kind in items)
        print(f"{len(items):4d} {symbol:48s} {references}")


if __name__ == "__main__":
    main()

