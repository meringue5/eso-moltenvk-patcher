#!/usr/bin/env python3

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


VULKAN_PIPELINE_CACHE_HEADER_LENGTH = 32
VULKAN_PIPELINE_CACHE_HEADER_VERSION_ONE = 1


class PipelineCacheIdentityError(ValueError):
    pass


@dataclass(frozen=True)
class PipelineCacheIdentity:
    header_length: int
    header_version: int
    vendor_id: int
    device_id: int
    uuid: bytes


def read_pipeline_cache_identity(path: Path) -> PipelineCacheIdentity:
    try:
        with path.open("rb") as stream:
            header = stream.read(VULKAN_PIPELINE_CACHE_HEADER_LENGTH)
    except OSError as error:
        raise PipelineCacheIdentityError(f"cannot read pipeline cache: {error}") from error

    if len(header) != VULKAN_PIPELINE_CACHE_HEADER_LENGTH:
        raise PipelineCacheIdentityError(
            "pipeline cache is shorter than the 32-byte Vulkan header"
        )

    header_length, header_version, vendor_id, device_id = struct.unpack(
        "<IIII", header[:16]
    )
    if header_length != VULKAN_PIPELINE_CACHE_HEADER_LENGTH:
        raise PipelineCacheIdentityError(
            f"unexpected header length {header_length}, expected 32"
        )
    if header_version != VULKAN_PIPELINE_CACHE_HEADER_VERSION_ONE:
        raise PipelineCacheIdentityError(
            f"unexpected header version {header_version}, expected 1"
        )

    return PipelineCacheIdentity(
        header_length=header_length,
        header_version=header_version,
        vendor_id=vendor_id,
        device_id=device_id,
        uuid=header[16:32],
    )


def parse_expected_uuid(value: str) -> bytes:
    if len(value) != 32:
        raise PipelineCacheIdentityError(
            "expected UUID must contain exactly 32 hexadecimal characters"
        )
    try:
        return bytes.fromhex(value)
    except ValueError as error:
        raise PipelineCacheIdentityError(
            "expected UUID must contain only hexadecimal characters"
        ) from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Read and verify a Vulkan pipeline-cache identity header."
    )
    parser.add_argument("cache", type=Path)
    parser.add_argument("--expect-uuid")
    arguments = parser.parse_args(argv)

    try:
        identity = read_pipeline_cache_identity(arguments.cache)
        if arguments.expect_uuid is not None:
            expected_uuid = parse_expected_uuid(arguments.expect_uuid)
            if identity.uuid != expected_uuid:
                raise PipelineCacheIdentityError(
                    "pipeline-cache UUID mismatch: "
                    f"expected {expected_uuid.hex()}, actual {identity.uuid.hex()}"
                )
    except PipelineCacheIdentityError as error:
        print(f"pipeline cache identity: FAIL ({error})", file=sys.stderr)
        return 1

    print(f"header_length={identity.header_length}")
    print(f"header_version={identity.header_version}")
    print(f"vendor_id=0x{identity.vendor_id:08x}")
    print(f"device_id=0x{identity.device_id:08x}")
    print(f"uuid={identity.uuid.hex()}")
    print("pipeline cache identity: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
