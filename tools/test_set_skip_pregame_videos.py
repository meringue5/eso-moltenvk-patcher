#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "set-skip-pregame-videos.sh"


class SkipPregameVideosTests(unittest.TestCase):
    def run_script(self, live: Path, value: str) -> subprocess.CompletedProcess[str]:
        fake_bin = live / "test-bin"
        fake_bin.mkdir(exist_ok=True)
        fake_pgrep = fake_bin / "pgrep"
        fake_pgrep.write_text("#!/bin/sh\nexit 1\n")
        fake_pgrep.chmod(0o755)
        environment = os.environ.copy()
        environment["ESO_LIVE"] = str(live)
        environment["PATH"] = f"{fake_bin}:{environment['PATH']}"
        return subprocess.run(
            [str(SCRIPT), value],
            check=False,
            capture_output=True,
            text=True,
            env=environment,
        )

    def test_replaces_only_skip_pregame_and_preserves_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            live = Path(directory)
            settings = live / "UserSettings.txt"
            original = (
                'SET VIDEO_ENABLED "1"\n'
                'SET SkipPregameVideos "0"\n'
                'SET AMBIENT_OCCLUSION_TYPE "0"\n'
            )
            settings.write_text(original)

            result = self.run_script(live, "1")

            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertEqual(
                settings.read_text(),
                original.replace(
                    'SET SkipPregameVideos "0"', 'SET SkipPregameVideos "1"'
                ),
            )
            backups = list(
                live.glob("UserSettings.txt.teso4m4-before-skip-pregame-0-to-1-*")
            )
            self.assertEqual(len(backups), 1)
            self.assertEqual(backups[0].read_text(), original)

            no_op = self.run_script(live, "1")
            self.assertEqual(no_op.returncode, 0, no_op.stderr + no_op.stdout)
            self.assertEqual(
                list(
                    live.glob(
                        "UserSettings.txt.teso4m4-before-skip-pregame-0-to-1-*"
                    )
                ),
                backups,
            )

    def test_rejects_ambiguous_or_unsupported_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            live = Path(directory)
            settings = live / "UserSettings.txt"
            duplicate = (
                'SET SkipPregameVideos "0"\n'
                'SET SkipPregameVideos "1"\n'
            )
            settings.write_text(duplicate)

            ambiguous = self.run_script(live, "1")
            self.assertNotEqual(ambiguous.returncode, 0)
            self.assertEqual(settings.read_text(), duplicate)
            self.assertEqual(list(live.glob("UserSettings.txt.teso4m4-before-*")), [])

            unsupported = self.run_script(live, "2")
            self.assertEqual(unsupported.returncode, 2)
            self.assertEqual(settings.read_text(), duplicate)

    def test_rejects_running_or_uninspectable_process_state(self) -> None:
        for process_exit in (0, 3):
            with self.subTest(process_exit=process_exit):
                with tempfile.TemporaryDirectory() as directory:
                    live = Path(directory)
                    settings = live / "UserSettings.txt"
                    original = 'SET SkipPregameVideos "0"\n'
                    settings.write_text(original)
                    fake_bin = live / "test-bin"
                    fake_bin.mkdir()
                    fake_pgrep = fake_bin / "pgrep"
                    fake_pgrep.write_text(
                        f"#!/bin/sh\nexit {process_exit}\n"
                    )
                    fake_pgrep.chmod(0o755)
                    environment = os.environ.copy()
                    environment["ESO_LIVE"] = str(live)
                    environment["PATH"] = f"{fake_bin}:{environment['PATH']}"

                    result = subprocess.run(
                        [str(SCRIPT), "1"],
                        check=False,
                        capture_output=True,
                        text=True,
                        env=environment,
                    )

                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(settings.read_text(), original)
                    self.assertEqual(
                        list(live.glob("UserSettings.txt.teso4m4-before-*")), []
                    )


if __name__ == "__main__":
    unittest.main()
