#!/usr/bin/env python3

from __future__ import annotations

import argparse
from contextlib import redirect_stdout
import io
from pathlib import Path
import tempfile
import unittest

from launcher_state import command_check, latest_live_snapshot


def repository(name: str, local: str, remote: str) -> str:
    return (
        "07/25/2026 18:51:38 CONSOLE - --- RepositoryCheck [Live_Prod] "
        f"{name} localId:{local} remoteId:{remote} | redacted ---"
    )


COMPLETE = (
    '07/25/2026 18:51:38 CONSOLE - WorkflowManager - Event Complete - '
    '{"successful":true,"cancelled":false,"name":"stateCheck",'
    '"workflow":"example","exists":true,"global":true}'
)
REPOSITORIES = [
    "PCPublicClientData",
    "PublicCrashReporterConfig",
    "DefaultPublicPlatformsConfig",
    "AppSettingsConfig",
    "MacPubPlayerClient",
    "shared_vo_soundsets",
    "shared_vo_en",
    "public_depot",
]


def completed_snapshot(
    *,
    mismatch: str | None = None,
    no_update: bool = True,
) -> str:
    lines = ["Running subtask noUpdateRequired"] if no_update else []
    for index, name in enumerate(REPOSITORIES):
        local = f"a{index}"
        remote = "ff" if name == mismatch else local
        lines.append(repository(name, local, remote))
    lines.append(COMPLETE)
    return "\n".join(lines)


class LauncherStateTests(unittest.TestCase):
    def test_accepts_completed_matching_snapshot(self) -> None:
        text = completed_snapshot()
        snapshot = latest_live_snapshot(text)
        self.assertIsNotNone(snapshot)
        assert snapshot is not None
        self.assertTrue(snapshot.current)
        self.assertEqual(len(snapshot.repositories), 8)

    def test_rejects_partial_snapshot(self) -> None:
        text = "\n".join(
            [
                "Running subtask noUpdateRequired",
                repository("public_depot", "a1", "a1"),
            ]
        )
        self.assertIsNone(latest_live_snapshot(text))

    def test_reports_repository_mismatch(self) -> None:
        text = completed_snapshot(mismatch="public_depot")
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "host.developer.example.log"
            log.write_text(text, encoding="utf-8")
            args = argparse.Namespace(log=log, log_dir=None)
            output = io.StringIO()
            with redirect_stdout(output):
                self.assertEqual(command_check(args), 3)
            self.assertIn("UPDATE_OR_ERROR", output.getvalue())
            self.assertIn("MISMATCH", output.getvalue())

    def test_requires_launcher_no_update_verdict(self) -> None:
        text = completed_snapshot(no_update=False)
        snapshot = latest_live_snapshot(text)
        self.assertIsNotNone(snapshot)
        assert snapshot is not None
        self.assertFalse(snapshot.current)

    def test_does_not_inherit_prior_no_update_verdict(self) -> None:
        text = "\n".join(
            [
                completed_snapshot(),
                completed_snapshot(no_update=False),
            ]
        )
        snapshot = latest_live_snapshot(text)
        self.assertIsNotNone(snapshot)
        assert snapshot is not None
        self.assertFalse(snapshot.current)

    def test_ignores_newer_lightweight_snapshot(self) -> None:
        text = "\n".join(
            [
                completed_snapshot(),
                "Running subtask noUpdateRequired",
                repository("MacPubPlayerClient", "a1", "a1"),
                repository("public_depot", "b2", "b2"),
                COMPLETE,
            ]
        )
        snapshot = latest_live_snapshot(text)
        self.assertIsNotNone(snapshot)
        assert snapshot is not None
        self.assertEqual(len(snapshot.repositories), 8)


if __name__ == "__main__":
    unittest.main()
