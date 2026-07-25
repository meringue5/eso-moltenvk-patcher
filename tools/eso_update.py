#!/usr/bin/env python3
"""Detect ESO updates and fail-closed audit unchanged-layout target rebases.

The fast rebase path is intentionally narrow.  It accepts a new executable
only when the embedded MoltenVK object, patch sites, external-reference shape,
and direct GIPA/GDPA query shape match a profiled reference manifest exactly.
It never modifies the game bundle.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
from typing import Any
import uuid

from analyze_vk_calls import (
    Reference,
    find_section,
    image_base,
    load_exports,
    load_sections,
    load_symbols,
    scan_dyld_fixups,
    scan_text,
)
from analyze_vk_proc_queries import find_queries
from generate_targets import PATCH_SIZE, macho_uuid


SCHEMA_VERSION = 2
DEFAULT_MEMBER = "MoltenVK-x86_64-master.o"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot load target manifest {path}: {error}") from error
    for key in ("sha256", "uuid", "proc_addr_slots", "targets"):
        if key not in manifest:
            raise ValueError(f"target manifest lacks {key}: {path}")
    return manifest


def client_build_identity(path: Path) -> dict[str, str]:
    try:
        lines = path.read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeDecodeError) as error:
        raise ValueError(f"cannot read client databuild stamp {path}: {error}") from error
    if len(lines) < 3:
        raise ValueError(f"invalid client databuild stamp: {path}")
    prefix = lines[0].split(".")
    try:
        live_index = prefix.index("live")
        databuild = prefix[live_index + 1]
    except (ValueError, IndexError) as error:
        raise ValueError(f"invalid client databuild identity: {path}") from error
    version = lines[2].strip()
    if not databuild.isdigit() or not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError(f"invalid client build values: {path}")
    return {"version": version, "databuild": databuild}


def executable_identity(path: Path) -> tuple[str, str]:
    data = path.read_bytes()
    return sha256_bytes(data), str(uuid.UUID(bytes=macho_uuid(data)))


def archive_member(archive: Path, member: str) -> bytes:
    result = subprocess.run(
        ["ar", "-p", str(archive), member],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode or not result.stdout:
        detail = result.stderr.decode("utf-8", "replace").strip()
        raise ValueError(
            f"cannot extract {member} from {archive}"
            + (f": {detail}" if detail else "")
        )
    return result.stdout


def archive_member_hashes(archive: Path) -> dict[str, str]:
    result = subprocess.run(
        ["ar", "-t", str(archive)],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode:
        raise ValueError(
            f"cannot list archive {archive}: {result.stderr.strip()}"
        )
    members = [
        name.strip()
        for name in result.stdout.splitlines()
        if name.strip() and not name.strip().startswith("__.SYMDEF")
    ]
    if not members or len(set(members)) != len(members):
        raise ValueError(f"archive has no unique object-member list: {archive}")
    return {
        member: sha256_bytes(archive_member(archive, member))
        for member in members
    }


def patch_targets(
    executable: bytes,
    manifest: dict[str, Any],
    symbol_offsets: dict[str, int],
) -> tuple[list[dict[str, Any]], list[str]]:
    rendered = []
    failures = []
    seen: set[str] = set()
    for target in manifest["targets"]:
        symbol = target.get("symbol")
        if not isinstance(symbol, str) or symbol in seen:
            failures.append(f"invalid or duplicate patch symbol: {symbol!r}")
            continue
        seen.add(symbol)
        try:
            offset = int(target["image_offset"], 0)
        except (KeyError, TypeError, ValueError):
            failures.append(f"invalid patch offset for {symbol}")
            continue
        actual = executable[offset : offset + PATCH_SIZE]
        if len(actual) != PATCH_SIZE:
            failures.append(f"patch site outside executable: {symbol}")
            continue
        expected_offset = symbol_offsets.get(symbol)
        if expected_offset != offset:
            found = "missing" if expected_offset is None else f"0x{expected_offset:x}"
            failures.append(
                f"patch symbol address changed: {symbol} manifest=0x{offset:x} object={found}"
            )
        expected_bytes = target.get("expected_bytes")
        if expected_bytes is not None:
            if not isinstance(expected_bytes, str) or len(expected_bytes) != PATCH_SIZE * 2:
                failures.append(f"invalid expected patch bytes for {symbol}")
            elif expected_bytes.lower() != actual.hex():
                failures.append(
                    f"patch bytes changed: {symbol} expected={expected_bytes.lower()} "
                    f"actual={actual.hex()}"
                )
        rendered.append(
            {
                "symbol": symbol,
                "image_offset": f"0x{offset:x}",
                "expected_bytes": actual.hex(),
            }
        )
    return rendered, failures


def reference_shape(references: dict[str, set[Reference]]) -> dict[str, Any]:
    by_symbol = {}
    for symbol, items in sorted(references.items()):
        kinds = Counter(item.kind for item in items)
        by_symbol[symbol] = {
            "total": len(items),
            "kinds": dict(sorted(kinds.items())),
            "sites": [
                f"0x{item.source:x}:{item.kind}"
                for item in sorted(items, key=lambda item: (item.source, item.kind))
            ],
        }
    return {
        "total": sum(len(items) for items in references.values()),
        "by_symbol": by_symbol,
    }


def query_shape(executable: Path, manifest: dict[str, Any]) -> dict[str, Any]:
    slots = manifest["proc_addr_slots"]
    queries, unknown = find_queries(
        executable,
        {
            int(slots["vkGetInstanceProcAddr"], 0): "GIPA",
            int(slots["vkGetDeviceProcAddr"], 0): "GDPA",
        },
    )
    routes = {}
    for route in ("GIPA", "GDPA"):
        items = queries.get(route, [])
        routes[route] = {
            "sites": len(items),
            "names": sorted({name for _, name in items}),
            "queries": [
                f"0x{source:x}:{name}"
                for source, name in sorted(items)
            ],
        }
    return {
        "routes": routes,
        "unnamed_sites": len(unknown),
    }


def analyze_layout(
    executable: Path,
    object_bytes: bytes,
    manifest: dict[str, Any],
    link_delta: int,
    member: str = DEFAULT_MEMBER,
) -> tuple[dict[str, Any], list[dict[str, Any]], list[str]]:
    with tempfile.TemporaryDirectory(prefix="teso4m4-update-") as directory:
        object_path = Path(directory) / Path(member).name
        object_path.write_bytes(object_bytes)

        executable_sections = load_sections(executable)
        object_sections = load_sections(object_path)
        text = find_section(executable_sections, "__TEXT", "__text")
        object_text = find_section(object_sections, None, "__text")
        base = image_base(executable_sections)
        symbols = load_symbols(object_path, link_delta, base)
        old_text_start = base + link_delta + object_text.address
        old_text_end = old_text_start + object_text.size
        references: dict[str, set[Reference]] = defaultdict(set)
        scan_text(
            executable.read_bytes(),
            text,
            symbols,
            references,
            old_text_start,
            old_text_end,
        )
        scan_dyld_fixups(
            executable, symbols, references, old_text_start, old_text_end
        )

    symbol_offsets = {name: address - base for address, name in symbols.items()}
    targets, failures = patch_targets(
        executable.read_bytes(), manifest, symbol_offsets
    )
    target_symbols = {target["symbol"] for target in targets}
    referenced_symbols = set(references)
    missing_targets = sorted(referenced_symbols - target_symbols)
    unreferenced_targets = sorted(target_symbols - referenced_symbols)
    if missing_targets:
        failures.append(
            "referenced entry points absent from target list: "
            + ", ".join(missing_targets)
        )
    if unreferenced_targets:
        failures.append(
            "target entries without external references: "
            + ", ".join(unreferenced_targets)
        )
    analysis = {
        "legacy_moltenvk": {
            "archive_member": member,
            "object_sha256": sha256_bytes(object_bytes),
            "link_delta": f"0x{link_delta:x}",
            "vulkan_text_symbols": len(symbols),
        },
        "external_references": reference_shape(references),
        "proc_queries": query_shape(executable, manifest),
    }
    return analysis, targets, failures


def compare_analysis(
    reference: dict[str, Any], actual: dict[str, Any]
) -> list[str]:
    failures = []
    for key in (
        "legacy_moltenvk",
        "replacement_runtime",
        "external_references",
        "proc_queries",
    ):
        if reference.get(key) != actual.get(key):
            failures.append(f"analysis profile changed: {key}")
    return failures


def profile_manifest(
    executable: Path,
    archive: Path,
    manifest: dict[str, Any],
    member: str,
    link_delta: int,
    new_runtime: Path,
) -> dict[str, Any]:
    actual_sha, actual_uuid = executable_identity(executable)
    if actual_sha != manifest["sha256"] or actual_uuid != str(
        uuid.UUID(manifest["uuid"])
    ):
        raise ValueError("reference manifest does not identify the local executable")
    object_bytes = archive_member(archive, member)
    analysis, targets, failures = analyze_layout(
        executable, object_bytes, manifest, link_delta, member
    )
    if failures:
        raise ValueError("; ".join(failures))
    analysis["legacy_moltenvk"]["archive_members"] = archive_member_hashes(
        archive
    )
    analysis["replacement_runtime"] = {
        "sha256": sha256_path(new_runtime),
        "vulkan_exports": len(load_exports(new_runtime)),
    }
    profiled = dict(manifest)
    profiled["schema_version"] = SCHEMA_VERSION
    profiled["uuid"] = actual_uuid
    profiled["analysis"] = analysis
    profiled["targets"] = targets
    return profiled


def audit_manifest(
    executable: Path,
    archive: Path,
    reference: dict[str, Any],
    new_runtime: Path,
    description: str,
) -> tuple[dict[str, Any] | None, list[str]]:
    if reference.get("schema_version") != SCHEMA_VERSION or "analysis" not in reference:
        return None, ["reference manifest has no schema-v2 update profile"]
    legacy = reference["analysis"]["legacy_moltenvk"]
    member = legacy["archive_member"]
    try:
        identity_before = executable_identity(executable)
        object_bytes = archive_member(archive, member)
        actual_analysis, targets, failures = analyze_layout(
            executable,
            object_bytes,
            reference,
            int(legacy["link_delta"], 0),
            member,
        )
        actual_analysis["legacy_moltenvk"]["archive_members"] = (
            archive_member_hashes(archive)
        )
        runtime_exports = load_exports(new_runtime)
        actual_analysis["replacement_runtime"] = {
            "sha256": sha256_path(new_runtime),
            "vulkan_exports": len(runtime_exports),
        }
    except (OSError, subprocess.SubprocessError, ValueError) as error:
        return None, [str(error)]
    failures.extend(compare_analysis(reference["analysis"], actual_analysis))
    if executable_identity(executable) != identity_before:
        failures.append("ESO executable changed while the audit was running")
    if (
        archive_member_hashes(archive)
        != actual_analysis["legacy_moltenvk"]["archive_members"]
    ):
        failures.append("embedded MoltenVK archive changed while the audit was running")
    if sha256_path(new_runtime) != actual_analysis["replacement_runtime"]["sha256"]:
        failures.append("replacement runtime changed while the audit was running")

    referenced = set(actual_analysis["external_references"]["by_symbol"])
    try:
        unavailable = sorted(referenced - runtime_exports)
    except (OSError, subprocess.SubprocessError) as error:
        failures.append(f"cannot inspect replacement runtime exports: {error}")
        unavailable = []
    if unavailable:
        failures.append(
            "replacement runtime lacks referenced entry points: "
            + ", ".join(unavailable)
        )
    if failures:
        return None, failures

    actual_sha, actual_uuid = identity_before
    candidate = dict(reference)
    candidate["description"] = description
    candidate["sha256"] = actual_sha
    candidate["uuid"] = actual_uuid
    candidate["derived_from_sha256"] = reference["sha256"]
    candidate["analysis"] = actual_analysis
    candidate["targets"] = targets
    return candidate, []


def write_json(path: Path, value: dict[str, Any], *, refuse_existing: bool) -> None:
    if refuse_existing and path.exists():
        raise ValueError(f"refusing to overwrite existing file: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".writing")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def command_check(args: argparse.Namespace) -> int:
    actual_sha, actual_uuid = executable_identity(args.exe)
    matches = []
    for path in sorted(args.manifest_dir.glob("targets-eso-*.json")):
        manifest = load_manifest(path)
        if manifest["sha256"] == actual_sha:
            matches.append((path, manifest))
    current = load_manifest(args.current_manifest)
    stamp = getattr(args, "databuild_stamp", None)
    actual_client = client_build_identity(stamp) if stamp else None
    expected_client = current.get("client_build")
    content_current = expected_client is None or actual_client == expected_client
    if (
        current["sha256"] == actual_sha
        and str(uuid.UUID(current["uuid"])) == actual_uuid
        and content_current
    ):
        state = "CURRENT"
        match = args.current_manifest
        code = 0
    elif (
        current["sha256"] == actual_sha
        and str(uuid.UUID(current["uuid"])) == actual_uuid
    ):
        state = "CONTENT_UPDATE"
        match = args.current_manifest
        code = 3
    elif matches:
        valid = [
            path
            for path, manifest in matches
            if str(uuid.UUID(manifest["uuid"])) == actual_uuid
        ]
        state = "KNOWN_OTHER" if valid else "IDENTITY_MISMATCH"
        match = valid[0] if valid else matches[0][0]
        code = 3
    else:
        state = "UNKNOWN_UPDATE"
        match = None
        code = 3
    print(f"ESO update status: {state}")
    print(f"ESO SHA-256: {actual_sha}")
    print(f"ESO UUID: {actual_uuid}")
    if actual_client is not None:
        print(f"ESO client version: {actual_client['version']}")
        print(f"ESO databuild: {actual_client['databuild']}")
        print(
            "ESO content status: "
            + ("CURRENT" if content_current else "UNKNOWN_UPDATE")
        )
    print(f"Current target: {args.current_manifest.name}")
    if match:
        print(f"Matched target: {match.name}")
    else:
        print("Matched target: none")
    return code


def command_profile(args: argparse.Namespace) -> int:
    manifest = load_manifest(args.manifest)
    profiled = profile_manifest(
        args.exe,
        args.archive,
        manifest,
        args.member,
        int(args.link_delta, 0),
        args.new_runtime,
    )
    write_json(args.output, profiled, refuse_existing=False)
    print(f"Profiled target manifest: {args.output}")
    return 0


def command_audit(args: argparse.Namespace) -> int:
    reference = load_manifest(args.reference)
    stamp = getattr(args, "databuild_stamp", None)
    client_before = client_build_identity(stamp) if stamp else None
    candidate, failures = audit_manifest(
        args.exe,
        args.archive,
        reference,
        args.new_runtime,
        args.description,
    )
    if stamp and client_build_identity(stamp) != client_before:
        failures.append("client databuild stamp changed while the audit was running")
    if candidate is not None and client_before is not None:
        candidate["client_build"] = client_before
    if failures:
        print("ESO update audit: MANUAL_ANALYSIS_REQUIRED")
        for failure in failures:
            print(f"FAIL: {failure}")
        return 3
    assert candidate is not None
    write_json(args.output, candidate, refuse_existing=not args.force)
    print("ESO update audit: FAST_REBASE_SAFE")
    print(f"Candidate target: {args.output}")
    print(f"ESO SHA-256: {candidate['sha256']}")
    print(f"ESO UUID: {candidate['uuid']}")
    print("Game bundle modified: no")
    return 0


def command_select(args: argparse.Namespace) -> int:
    candidate_path = args.candidate.resolve()
    config_dir = args.pointer.resolve().parent
    if candidate_path.parent != config_dir or not candidate_path.name.startswith(
        "targets-eso-"
    ) or candidate_path.suffix != ".json":
        raise ValueError("candidate must be a targets-eso-*.json file beside the pointer")
    candidate = load_manifest(candidate_path)
    actual_sha, actual_uuid = executable_identity(args.exe)
    if candidate.get("schema_version") != SCHEMA_VERSION or "analysis" not in candidate:
        raise ValueError("candidate lacks the schema-v2 update profile")
    if candidate["sha256"] != actual_sha or str(uuid.UUID(candidate["uuid"])) != actual_uuid:
        raise ValueError("candidate does not identify the local executable")
    stamp = getattr(args, "databuild_stamp", None)
    if stamp and candidate.get("client_build") != client_build_identity(stamp):
        raise ValueError("candidate does not identify the local client build")
    temporary = args.pointer.with_name(args.pointer.name + ".writing")
    temporary.write_text(candidate_path.name + "\n", encoding="ascii")
    temporary.replace(args.pointer)
    print(f"Selected current target: {candidate_path.name}")
    print("Game bundle modified: no")
    return 0


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser()
    commands = root.add_subparsers(dest="command", required=True)

    check = commands.add_parser("check")
    check.add_argument("--exe", required=True, type=Path)
    check.add_argument("--manifest-dir", required=True, type=Path)
    check.add_argument("--current-manifest", required=True, type=Path)
    check.add_argument("--databuild-stamp", type=Path)
    check.set_defaults(function=command_check)

    profile = commands.add_parser("profile")
    profile.add_argument("--exe", required=True, type=Path)
    profile.add_argument("--archive", required=True, type=Path)
    profile.add_argument("--manifest", required=True, type=Path)
    profile.add_argument("--output", required=True, type=Path)
    profile.add_argument("--member", default=DEFAULT_MEMBER)
    profile.add_argument("--link-delta", default="0x1a08490")
    profile.add_argument("--new-runtime", required=True, type=Path)
    profile.set_defaults(function=command_profile)

    audit = commands.add_parser("audit")
    audit.add_argument("--exe", required=True, type=Path)
    audit.add_argument("--archive", required=True, type=Path)
    audit.add_argument("--reference", required=True, type=Path)
    audit.add_argument("--new-runtime", required=True, type=Path)
    audit.add_argument("--description", required=True)
    audit.add_argument("--output", required=True, type=Path)
    audit.add_argument("--databuild-stamp", type=Path)
    audit.add_argument("--force", action="store_true")
    audit.set_defaults(function=command_audit)

    select = commands.add_parser("select")
    select.add_argument("--exe", required=True, type=Path)
    select.add_argument("--candidate", required=True, type=Path)
    select.add_argument("--pointer", required=True, type=Path)
    select.add_argument("--databuild-stamp", type=Path)
    select.set_defaults(function=command_select)
    return root


def main() -> None:
    args = parser().parse_args()
    try:
        result = args.function(args)
    except (OSError, subprocess.SubprocessError, ValueError) as error:
        print(f"ERROR: {error}")
        raise SystemExit(2) from error
    raise SystemExit(result)


if __name__ == "__main__":
    main()
