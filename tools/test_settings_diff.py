#!/usr/bin/env python3

from __future__ import annotations

from contextlib import redirect_stdout
import io
import unittest
from pathlib import Path
import tempfile

from settings_diff import (
    command_compare,
    compare,
    parse_expected,
    sha256_text,
)


class SettingsDiffTests(unittest.TestCase):
    def test_normalizes_expected_value_and_reports_only_other_changes(self) -> None:
        reference = (
            'SET SkipPregameVideos "0"\n'
            'SET FullscreenWidth "2048"\n'
            'SET FullscreenHeight "1280"\n'
        )
        actual = (
            'SET SkipPregameVideos "1"\n'
            'SET FullscreenWidth "1920"\n'
            'SET FullscreenHeight "1200"\n'
        )
        normalized, changes = compare(
            reference,
            actual,
            {"SkipPregameVideos": "1"},
        )
        self.assertEqual(
            changes,
            [
                ("FullscreenWidth", "2048", "1920"),
                ("FullscreenHeight", "1280", "1200"),
            ],
        )
        self.assertEqual(
            sha256_text(normalized),
            sha256_text(reference.replace('"0"', '"1"', 1)),
        )

    def test_rejects_structure_change(self) -> None:
        with self.assertRaises(ValueError):
            compare('SET A "1"\n', 'SET A "1"\nextra\n', {})

    def test_rejects_duplicate_expected_key(self) -> None:
        with self.assertRaises(ValueError):
            parse_expected(["A=1", "A=2"])

    def test_command_writes_normalized_reference_without_overwrite(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            reference = root / "before.txt"
            actual = root / "after.txt"
            output = root / "normalized.txt"
            reference.write_bytes(b'SET A "0"\r\n')
            actual.write_bytes(b'SET A "1"\r\n')
            args = type(
                "Args",
                (),
                {
                    "reference": reference,
                    "actual": actual,
                    "expected": ["A=1"],
                    "normalized_output": output,
                },
            )()
            with redirect_stdout(io.StringIO()):
                self.assertEqual(command_compare(args), 0)
            self.assertEqual(output.read_bytes(), b'SET A "1"\r\n')
            with self.assertRaises(ValueError):
                with redirect_stdout(io.StringIO()):
                    command_compare(args)


if __name__ == "__main__":
    unittest.main()
