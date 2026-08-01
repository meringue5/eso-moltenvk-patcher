import os
import shlex
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LIB_TARGET = ROOT / "scripts" / "lib-target.sh"


class BundleIdleGateTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.fake_bin = self.root / "bin"
        self.fake_bin.mkdir()
        self.eso_app = self.root / "steamapps" / "common" / "ESO" / "eso.app"
        self.eso_app.mkdir(parents=True)
        self.steamapps = self.root / "steamapps"
        self.manifest = self.steamapps / "appmanifest_306130.acf"
        self.manifest.write_text(
            '"AppState"\n{\n'
            '\t"StateFlags"\t\t"4"\n'
            '\t"BytesToDownload"\t\t"100"\n'
            '\t"BytesDownloaded"\t\t"100"\n'
            '}\n',
            encoding="utf-8",
        )
        pgrep = self.fake_bin / "pgrep"
        pgrep.write_text(
            "#!/bin/sh\n"
            "case \"$*\" in\n"
            "  '-x eso') exit \"${FAKE_ESO_STATUS:-1}\" ;;\n"
            "  *'ZeniMax Online Studios Launcher'*) "
            "exit \"${FAKE_LAUNCHER_STATUS:-1}\" ;;\n"
            "  *'Steam/Contents/MacOS/steam_osx'*) "
            "exit \"${FAKE_STEAM_STATUS:-1}\" ;;\n"
            "esac\n"
            "exit 1\n",
            encoding="utf-8",
        )
        pgrep.chmod(0o755)
        lsof = self.fake_bin / "lsof"
        lsof.write_text(
            "#!/bin/sh\n"
            "if [ \"${FAKE_LSOF_ERROR:-0}\" = 1 ]; then exit 2; fi\n"
            "if [ -n \"${FAKE_LSOF_FILE:-}\" ]; then\n"
            "  printf 'p123\\nf%s\\n' \"$FAKE_LSOF_FILE\"\n"
            "  exit 0\n"
            "fi\n"
            "exit 1\n",
            encoding="utf-8",
        )
        lsof.chmod(0o755)

    def tearDown(self):
        self.temporary.cleanup()

    def run_gate(self, **overrides):
        environment = os.environ.copy()
        environment.update(
            {
                "PATH": f"{self.fake_bin}:{environment['PATH']}",
                "TESO4M4_STEAMAPPS_ROOT": str(self.steamapps),
            }
        )
        environment.update(overrides)
        command = (
            f"source {shlex.quote(str(LIB_TARGET))}; "
            "teso4m4_require_bundle_idle "
            f"{shlex.quote(str(self.eso_app))}"
        )
        return subprocess.run(
            ["zsh", "-c", command],
            env=environment,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_idle_steam_is_allowed(self):
        result = self.run_gate(FAKE_STEAM_STATUS="0")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Steam is open", result.stdout)

    def test_eso_process_is_blocked(self):
        result = self.run_gate(FAKE_ESO_STATUS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ESO is running", result.stderr)

    def test_launcher_process_is_blocked(self):
        result = self.run_gate(FAKE_LAUNCHER_STATUS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ZeniMax launcher", result.stderr)

    def test_open_bundle_file_is_blocked(self):
        result = self.run_gate(
            FAKE_STEAM_STATUS="0",
            FAKE_LSOF_FILE=str(self.eso_app / "Contents" / "MacOS" / "eso"),
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("files open", result.stderr)

    def test_incomplete_steam_manifest_is_blocked(self):
        self.manifest.write_text(
            '"AppState"\n{\n'
            '\t"StateFlags"\t\t"1026"\n'
            '\t"BytesToDownload"\t\t"100"\n'
            '\t"BytesDownloaded"\t\t"20"\n'
            '}\n',
            encoding="utf-8",
        )
        result = self.run_gate(FAKE_STEAM_STATUS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("updating, incomplete", result.stderr)

    def test_staged_download_is_blocked(self):
        download = self.steamapps / "downloading" / "306130"
        download.mkdir(parents=True)
        (download / "chunk").write_text("pending", encoding="utf-8")
        result = self.run_gate(FAKE_STEAM_STATUS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("staged ESO download", result.stderr)

    def test_indeterminate_lsof_is_blocked(self):
        result = self.run_gate(FAKE_LSOF_ERROR="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Could not verify", result.stderr)


if __name__ == "__main__":
    unittest.main()
