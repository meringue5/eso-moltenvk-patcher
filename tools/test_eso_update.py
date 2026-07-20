#!/usr/bin/env python3

from __future__ import annotations

import argparse
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
import uuid

from eso_update import (
    SCHEMA_VERSION,
    command_check,
    command_select,
    compare_analysis,
    patch_targets,
    write_json,
)


MACH_HEADER_64 = struct.Struct("<IiiIIIII")
UUID_COMMAND = struct.Struct("<II16s")
MH_MAGIC_64 = 0xFEEDFACF
LC_UUID = 0x1B


def synthetic_macho(identifier: uuid.UUID, payload: bytes = b"") -> bytes:
    command = UUID_COMMAND.pack(LC_UUID, UUID_COMMAND.size, identifier.bytes)
    header = MACH_HEADER_64.pack(
        MH_MAGIC_64, 0x01000007, 3, 2, 1, len(command), 0, 0
    )
    return header + command + payload


def manifest_for(data: bytes, identifier: uuid.UUID) -> dict[str, object]:
    return {
        "description": "synthetic",
        "sha256": hashlib.sha256(data).hexdigest(),
        "uuid": str(identifier),
        "proc_addr_slots": {
            "vkGetInstanceProcAddr": "0x10",
            "vkGetDeviceProcAddr": "0x18",
        },
        "targets": [],
    }


class UpdateToolTests(unittest.TestCase):
    def test_patch_targets_records_and_validates_exact_bytes(self) -> None:
        executable = bytes(range(64))
        manifest = {
            "targets": [
                {
                    "symbol": "vkExample",
                    "image_offset": "0x10",
                    "expected_bytes": executable[16:28].hex(),
                }
            ]
        }
        targets, failures = patch_targets(
            executable, manifest, {"vkExample": 0x10}
        )
        self.assertEqual(failures, [])
        self.assertEqual(targets[0]["expected_bytes"], executable[16:28].hex())

        manifest["targets"][0]["expected_bytes"] = "00" * 12
        _, failures = patch_targets(executable, manifest, {"vkExample": 0x10})
        self.assertTrue(any("patch bytes changed" in item for item in failures))

    def test_patch_targets_rejects_symbol_offset_change(self) -> None:
        executable = bytes(range(64))
        manifest = {
            "targets": [
                {"symbol": "vkExample", "image_offset": "0x10"}
            ]
        }
        _, failures = patch_targets(
            executable, manifest, {"vkExample": 0x20}
        )
        self.assertTrue(any("symbol address changed" in item for item in failures))

    def test_analysis_comparison_is_exact_and_section_named(self) -> None:
        reference = {
            "legacy_moltenvk": {"object_sha256": "a"},
            "replacement_runtime": {"sha256": "b"},
            "external_references": {"total": 40},
            "proc_queries": {"unnamed_sites": 0},
        }
        self.assertEqual(compare_analysis(reference, dict(reference)), [])
        changed = dict(reference)
        changed["proc_queries"] = {"unnamed_sites": 1}
        self.assertEqual(
            compare_analysis(reference, changed),
            ["analysis profile changed: proc_queries"],
        )

    def test_check_distinguishes_current_and_unknown_update(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifests = root / "config"
            manifests.mkdir()
            identifier = uuid.uuid4()
            current_data = synthetic_macho(identifier, b"current")
            executable = root / "eso"
            executable.write_bytes(current_data)
            current = manifests / "targets-eso-current.json"
            current.write_text(json.dumps(manifest_for(current_data, identifier)))
            args = argparse.Namespace(
                exe=executable,
                manifest_dir=manifests,
                current_manifest=current,
            )
            with redirect_stdout(io.StringIO()):
                self.assertEqual(command_check(args), 0)

            executable.write_bytes(synthetic_macho(uuid.uuid4(), b"update"))
            with redirect_stdout(io.StringIO()):
                self.assertEqual(command_check(args), 3)

    def test_select_requires_profiled_exact_local_candidate(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "config"
            config.mkdir()
            identifier = uuid.uuid4()
            data = synthetic_macho(identifier)
            executable = Path(directory) / "eso"
            executable.write_bytes(data)
            candidate = config / "targets-eso-new.json"
            value = manifest_for(data, identifier)
            value["schema_version"] = SCHEMA_VERSION
            value["analysis"] = {"profile": "synthetic"}
            candidate.write_text(json.dumps(value))
            pointer = config / "current-target.txt"
            args = argparse.Namespace(
                exe=executable, candidate=candidate, pointer=pointer
            )
            with redirect_stdout(io.StringIO()):
                self.assertEqual(command_select(args), 0)
            self.assertEqual(pointer.read_text(), "targets-eso-new.json\n")

            executable.write_bytes(synthetic_macho(uuid.uuid4()))
            with self.assertRaises(ValueError):
                command_select(args)

    def test_write_json_refuses_to_replace_candidate(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.json"
            path.write_text("preserve")
            with self.assertRaises(ValueError):
                write_json(path, {"new": True}, refuse_existing=True)
            self.assertEqual(path.read_text(), "preserve")


if __name__ == "__main__":
    unittest.main()
