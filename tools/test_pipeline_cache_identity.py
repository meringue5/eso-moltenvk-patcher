#!/usr/bin/env python3

from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path

from pipeline_cache_identity import (
    PipelineCacheIdentityError,
    parse_expected_uuid,
    read_pipeline_cache_identity,
)


UUID = bytes.fromhex("db445ff21a0502090000000100000000")


def cache_header(
    *,
    header_length: int = 32,
    header_version: int = 1,
    uuid: bytes = UUID,
) -> bytes:
    return struct.pack("<IIII", header_length, header_version, 0x106B, 0x1234) + uuid


class PipelineCacheIdentityTests(unittest.TestCase):
    def write_cache(self, contents: bytes) -> Path:
        temporary = tempfile.NamedTemporaryFile(delete=False)
        self.addCleanup(Path(temporary.name).unlink, missing_ok=True)
        temporary.write(contents)
        temporary.close()
        return Path(temporary.name)

    def test_reads_valid_header(self) -> None:
        identity = read_pipeline_cache_identity(self.write_cache(cache_header()))
        self.assertEqual(identity.header_length, 32)
        self.assertEqual(identity.header_version, 1)
        self.assertEqual(identity.vendor_id, 0x106B)
        self.assertEqual(identity.device_id, 0x1234)
        self.assertEqual(identity.uuid, UUID)

    def test_expected_uuid_parser_accepts_exact_value(self) -> None:
        self.assertEqual(parse_expected_uuid(UUID.hex()), UUID)

    def test_expected_uuid_parser_rejects_wrong_length(self) -> None:
        with self.assertRaisesRegex(PipelineCacheIdentityError, "exactly 32"):
            parse_expected_uuid("00")

    def test_expected_uuid_parser_rejects_non_hexadecimal_value(self) -> None:
        with self.assertRaisesRegex(PipelineCacheIdentityError, "hexadecimal"):
            parse_expected_uuid("z" * 32)

    def test_rejects_truncated_header(self) -> None:
        with self.assertRaisesRegex(PipelineCacheIdentityError, "shorter"):
            read_pipeline_cache_identity(self.write_cache(cache_header()[:31]))

    def test_rejects_unknown_header_length(self) -> None:
        with self.assertRaisesRegex(PipelineCacheIdentityError, "header length"):
            read_pipeline_cache_identity(
                self.write_cache(cache_header(header_length=36))
            )

    def test_rejects_unknown_header_version(self) -> None:
        with self.assertRaisesRegex(PipelineCacheIdentityError, "header version"):
            read_pipeline_cache_identity(
                self.write_cache(cache_header(header_version=2))
            )


if __name__ == "__main__":
    unittest.main()
