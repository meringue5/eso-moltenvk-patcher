#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "set-ambient-occlusion.sh"


class SetAmbientOcclusionTests(unittest.TestCase):
    def run_script(self, live: Path, value: str) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        environment["ESO_LIVE"] = str(live)
        return subprocess.run(
            [str(SCRIPT), value],
            check=False,
            capture_output=True,
            text=True,
            env=environment,
        )

    def test_replaces_only_the_supported_line_and_preserves_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            live = Path(directory)
            settings = live / "UserSettings.txt"
            original = 'SET FOO "unchanged"\nSET AMBIENT_OCCLUSION_TYPE "1"\n'
            settings.write_text(original)

            result = self.run_script(live, "0")

            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual(
                settings.read_text(),
                'SET FOO "unchanged"\nSET AMBIENT_OCCLUSION_TYPE "0"\n',
            )
            backups = list(live.glob("UserSettings.txt.teso4m4-before-ao-1-to-0-*"))
            self.assertEqual(len(backups), 1)
            self.assertEqual(backups[0].read_text(), original)

            no_op = self.run_script(live, "0")
            self.assertEqual(no_op.returncode, 0, no_op.stderr + no_op.stdout)
            self.assertEqual(
                list(live.glob("UserSettings.txt.teso4m4-before-ao-1-to-0-*")),
                backups,
            )

    def test_rejects_ambiguous_or_unsupported_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            live = Path(directory)
            settings = live / "UserSettings.txt"
            duplicate = (
                'SET AMBIENT_OCCLUSION_TYPE "1"\n'
                'SET AMBIENT_OCCLUSION_TYPE "0"\n'
            )
            settings.write_text(duplicate)

            ambiguous = self.run_script(live, "0")
            self.assertNotEqual(ambiguous.returncode, 0)
            self.assertEqual(settings.read_text(), duplicate)
            self.assertEqual(list(live.glob("UserSettings.txt.teso4m4-before-*")), [])

            unsupported = self.run_script(live, "2")
            self.assertEqual(unsupported.returncode, 2)
            self.assertEqual(settings.read_text(), duplicate)


if __name__ == "__main__":
    unittest.main()
