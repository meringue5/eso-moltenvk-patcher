#!/usr/bin/env python3
"""Generate a patch table after validating an exact local ESO executable."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import uuid
from pathlib import Path


MACH_HEADER_64 = struct.Struct("<IiiIIIII")
LOAD_COMMAND = struct.Struct("<II")
MH_MAGIC_64 = 0xFEEDFACF
LC_UUID = 0x1B
PATCH_SIZE = 12


def macho_uuid(data: bytes) -> bytes:
    if len(data) < MACH_HEADER_64.size:
        raise ValueError("file is too small for a 64-bit Mach-O header")
    magic, _, _, _, command_count, _, _, _ = MACH_HEADER_64.unpack_from(data)
    if magic != MH_MAGIC_64:
        raise ValueError("expected a little-endian 64-bit Mach-O executable")
    cursor = MACH_HEADER_64.size
    for _ in range(command_count):
        command, command_size = LOAD_COMMAND.unpack_from(data, cursor)
        if command_size < LOAD_COMMAND.size or cursor + command_size > len(data):
            raise ValueError("invalid Mach-O load command")
        if command == LC_UUID:
            if command_size < 24:
                raise ValueError("invalid LC_UUID command")
            return data[cursor + 8 : cursor + 24]
        cursor += command_size
    raise ValueError("Mach-O has no LC_UUID command")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    executable = args.executable.read_bytes()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    actual_sha = hashlib.sha256(executable).hexdigest()
    if actual_sha != manifest["sha256"]:
        raise SystemExit(
            f"ESO SHA-256 mismatch\nexpected: {manifest['sha256']}\nactual:   {actual_sha}"
        )

    expected_uuid = uuid.UUID(manifest["uuid"]).bytes
    actual_uuid = macho_uuid(executable)
    if actual_uuid != expected_uuid:
        raise SystemExit(
            f"ESO UUID mismatch\nexpected: {uuid.UUID(bytes=expected_uuid)}\n"
            f"actual:   {uuid.UUID(bytes=actual_uuid)}"
        )

    uuid_bytes = ", ".join(f"0x{byte:02x}" for byte in expected_uuid)
    lines = [
        "/* Generated from a validated local ESO executable. Do not edit. */",
        "#pragma once",
        "",
        f'#define ESO_EXPECTED_SHA256 "{actual_sha}"',
        f"#define ESO_TARGET_COUNT {len(manifest['targets'])}",
        f"static const uint8_t kExpectedUUID[16] = {{{uuid_bytes}}};",
        "",
        "static const PatchTarget kPatchTargets[ESO_TARGET_COUNT] = {",
    ]
    for target in manifest["targets"]:
        offset = int(target["image_offset"], 0)
        expected = executable[offset : offset + PATCH_SIZE]
        if len(expected) != PATCH_SIZE:
            raise SystemExit(f"patch target outside executable: {target['symbol']}")
        recorded = target.get("expected_bytes")
        if manifest.get("schema_version", 1) >= 2:
            if not isinstance(recorded, str) or len(recorded) != PATCH_SIZE * 2:
                raise SystemExit(
                    f"invalid recorded patch bytes: {target['symbol']}"
                )
            if recorded.lower() != expected.hex():
                raise SystemExit(
                    f"recorded patch bytes mismatch: {target['symbol']}\n"
                    f"expected: {recorded.lower()}\nactual:   {expected.hex()}"
                )
        byte_list = ", ".join(f"0x{byte:02x}" for byte in expected)
        lines.append(
            f'    {{"{target["symbol"]}", 0x{offset:x}ULL, {{{byte_list}}}}},'
        )
    lines.extend(("};", ""))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")


if __name__ == "__main__":
    main()
