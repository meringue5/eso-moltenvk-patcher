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
FX_SENTINEL_PATCH_SIZE = 17
FX_SENTINEL_CONSTANT_SIZE = 16


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

    experimental = manifest.get("experimental_targets", {})
    sentinel = experimental.get("fx_material_initializer")
    if sentinel is None:
        lines.extend(
            (
                "#define ESO_HAS_FX_SENTINEL_TARGET 0",
                "static const Teso4m4FxSentinelTarget kFxSentinelTarget = {0};",
                "",
            )
        )
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text("\n".join(lines), encoding="ascii")
        return
    if not isinstance(sentinel, dict):
        raise SystemExit("FX material initializer profile must be an object")
    sentinel_offset = int(sentinel["image_offset"], 0)
    sentinel_expected = executable[
        sentinel_offset : sentinel_offset + FX_SENTINEL_PATCH_SIZE
    ]
    if len(sentinel_expected) != FX_SENTINEL_PATCH_SIZE:
        raise SystemExit("FX material initializer patch target is outside executable")
    recorded_sentinel = sentinel.get("expected_bytes")
    if (
        not isinstance(recorded_sentinel, str)
        or len(recorded_sentinel) != FX_SENTINEL_PATCH_SIZE * 2
        or recorded_sentinel.lower() != sentinel_expected.hex()
    ):
        raise SystemExit(
            "recorded FX material initializer bytes mismatch\n"
            f"expected: {recorded_sentinel}\nactual:   {sentinel_expected.hex()}"
        )
    constant_offset = int(sentinel["first_constant_offset"], 0)
    constant_expected = executable[
        constant_offset : constant_offset + FX_SENTINEL_CONSTANT_SIZE
    ]
    recorded_constant = sentinel.get("first_constant_expected_bytes")
    if (
        len(constant_expected) != FX_SENTINEL_CONSTANT_SIZE
        or not isinstance(recorded_constant, str)
        or len(recorded_constant) != FX_SENTINEL_CONSTANT_SIZE * 2
        or recorded_constant.lower() != constant_expected.hex()
    ):
        raise SystemExit(
            "recorded FX material constant bytes mismatch\n"
            f"expected: {recorded_constant}\nactual:   {constant_expected.hex()}"
        )
    callers = sentinel.get("caller_return_offsets")
    caller_calls = sentinel.get("caller_call_expected_bytes")
    if (
        not isinstance(callers, list)
        or not isinstance(caller_calls, list)
        or len(callers) != 2
        or len(caller_calls) != 2
    ):
        raise SystemExit("FX material initializer requires exactly two callers")
    caller_offsets = []
    for return_value, recorded_call in zip(callers, caller_calls, strict=True):
        return_offset = int(return_value, 0)
        call = executable[return_offset - 5 : return_offset]
        if (
            len(call) != 5
            or call[0] != 0xE8
            or not isinstance(recorded_call, str)
            or len(recorded_call) != 10
            or recorded_call.lower() != call.hex()
        ):
            raise SystemExit(
                f"recorded FX material caller bytes mismatch at 0x{return_offset:x}"
            )
        displacement = struct.unpack("<i", call[1:])[0]
        if return_offset + displacement != sentinel_offset:
            raise SystemExit(
                f"FX material caller does not target initializer at 0x{return_offset:x}"
            )
        caller_offsets.append(return_offset)

    sentinel_bytes = ", ".join(f"0x{byte:02x}" for byte in sentinel_expected)
    constant_bytes = ", ".join(f"0x{byte:02x}" for byte in constant_expected)
    lines.extend(
        (
            "#define ESO_HAS_FX_SENTINEL_TARGET 1",
            "static const Teso4m4FxSentinelTarget kFxSentinelTarget = {",
            f"    0x{sentinel_offset:x}ULL,",
            f"    {{{sentinel_bytes}}},",
            f"    0x{constant_offset:x}ULL,",
            f"    {{{constant_bytes}}},",
            "    {"
            + ", ".join(f"0x{offset:x}ULL" for offset in caller_offsets)
            + "},",
            "};",
            "",
        )
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")


if __name__ == "__main__":
    main()
