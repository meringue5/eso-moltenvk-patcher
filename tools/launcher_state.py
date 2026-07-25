#!/usr/bin/env python3
"""Read the launcher's last completed Live_Prod repository comparison.

This tool is deliberately passive.  It does not start the launcher, contact
the network, or trust a partial state check.  It only accepts a completed
Live_Prod snapshot already recorded by the launcher.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re


REPOSITORY_CHECK = re.compile(
    r"^(?P<timestamp>\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2}).*"
    r"RepositoryCheck \[Live_Prod\] (?P<name>\S+) "
    r"localId:(?P<local>[0-9a-f]+) remoteId:(?P<remote>[0-9a-f]+)"
)
STATE_CHECK_COMPLETE = re.compile(
    r'WorkflowManager - Event Complete - \{"successful":true,'
    r'"cancelled":false,"name":"stateCheck"'
)
NO_UPDATE_REQUIRED = "Running subtask noUpdateRequired"
REQUIRED_REPOSITORIES = {
    "PCPublicClientData",
    "PublicCrashReporterConfig",
    "DefaultPublicPlatformsConfig",
    "AppSettingsConfig",
    "MacPubPlayerClient",
    "shared_vo_soundsets",
    "public_depot",
}


@dataclass(frozen=True)
class RepositoryIdentity:
    name: str
    local: str
    remote: str


@dataclass(frozen=True)
class LiveSnapshot:
    timestamp: str
    repositories: tuple[RepositoryIdentity, ...]
    no_update_required: bool

    @property
    def complete_coverage(self) -> bool:
        names = {item.name for item in self.repositories}
        language_voice = any(
            name.startswith("shared_vo_") and name != "shared_vo_soundsets"
            for name in names
        )
        return REQUIRED_REPOSITORIES <= names and language_voice

    @property
    def current(self) -> bool:
        return self.complete_coverage and self.no_update_required and all(
            item.local == item.remote for item in self.repositories
        )


def latest_live_snapshot(text: str) -> LiveSnapshot | None:
    """Return the last full Live_Prod comparison, never a partial one."""

    lines = text.splitlines()
    snapshots: list[LiveSnapshot] = []
    for end, line in enumerate(lines):
        if not STATE_CHECK_COMPLETE.search(line):
            continue

        start = end - 1
        found: list[RepositoryIdentity] = []
        timestamp = ""
        while start >= 0:
            match = REPOSITORY_CHECK.search(lines[start])
            if match is None:
                break
            timestamp = match.group("timestamp")
            found.append(
                RepositoryIdentity(
                    name=match.group("name"),
                    local=match.group("local"),
                    remote=match.group("remote"),
                )
            )
            start -= 1
        if not found:
            continue

        found.reverse()
        names = [item.name for item in found]
        if len(names) != len(set(names)):
            continue
        nearby_start = 0
        for candidate_index in range(start, -1, -1):
            if STATE_CHECK_COMPLETE.search(lines[candidate_index]):
                nearby_start = candidate_index + 1
                break
        no_update = any(
            NO_UPDATE_REQUIRED in candidate
            for candidate in lines[nearby_start:end]
        )
        snapshots.append(LiveSnapshot(timestamp, tuple(found), no_update))
    for snapshot in reversed(snapshots):
        if snapshot.complete_coverage:
            return snapshot
    return None


def newest_host_log(log_dir: Path) -> Path:
    candidates = [
        path
        for path in log_dir.glob("host.developer.*.log")
        if not path.name.endswith(".selfupdate.log")
    ]
    if not candidates:
        raise ValueError(f"no launcher host log found in {log_dir}")
    return max(candidates, key=lambda path: path.stat().st_mtime_ns)


def command_check(args: argparse.Namespace) -> int:
    log = args.log or newest_host_log(args.log_dir)
    try:
        text = log.read_text(encoding="utf-8", errors="replace")
    except OSError as error:
        raise ValueError(f"cannot read launcher log {log}: {error}") from error

    snapshot = latest_live_snapshot(text)
    if snapshot is None:
        print("Launcher Live_Prod state: INDETERMINATE")
        print(f"Launcher log: {log.name}")
        print("Reason: no completed Live_Prod repository snapshot")
        return 3

    state = "CURRENT_REMOTE" if snapshot.current else "UPDATE_OR_ERROR"
    print(f"Launcher Live_Prod state: {state}")
    print(f"State check time: {snapshot.timestamp}")
    print(f"Repository count: {len(snapshot.repositories)}")
    print(f"Launcher log: {log.name}")
    for item in snapshot.repositories:
        relation = "MATCH" if item.local == item.remote else "MISMATCH"
        print(f"Repository {item.name}: {relation}")
    if not snapshot.no_update_required:
        print("Launcher verdict: no completed noUpdateRequired path")
    else:
        print("Launcher verdict: noUpdateRequired")
    return 0 if snapshot.current else 3


def parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    subcommands = value.add_subparsers(dest="command", required=True)
    check = subcommands.add_parser("check")
    check.add_argument("--log", type=Path)
    check.add_argument("--log-dir", type=Path)
    check.set_defaults(function=command_check)
    return value


def main() -> int:
    args = parser().parse_args()
    if args.log is None and args.log_dir is None:
        raise SystemExit("check requires --log or --log-dir")
    try:
        return args.function(args)
    except ValueError as error:
        raise SystemExit(str(error)) from error


if __name__ == "__main__":
    raise SystemExit(main())
