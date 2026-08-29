#!/usr/bin/env python3
"""Create and verify immutable fixture artifact identity metadata.

The canonical recipe is byte-defined: compact ASCII JSON followed by exactly
one LF.  The revision prefix is derived from those exact bytes, so callers
never hand-type a recipe hash or artifact directory name.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA = 1
FQBN = "esp32:esp32:esp32s3_powerfeather"
LIBRARY_NAMES = (
    "Adafruit BusIO",
    "Adafruit MSA301",
    "Adafruit NeoPixel",
    "Adafruit Unified Sensor",
    "SparkFun Qwiic TMF882X Library",
)


def fail(message: str) -> None:
    raise SystemExit(f"artifact identity error: {message}")


def run(command: list[str], *, cwd: Path | None = None) -> str:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode:
        tail = "\n".join(completed.stdout.splitlines()[-12:])
        fail(f"command failed ({' '.join(command)}):\n{tail}")
    return completed.stdout.replace("\r\n", "\n")


def canonical_recipe_bytes(recipe: dict[str, Any]) -> bytes:
    """Return the one canonical serialization used as the recipe hash input."""
    return (
        json.dumps(recipe, ensure_ascii=True, separators=(",", ":")) + "\n"
    ).encode("ascii")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def normalize_compile_flags(flag_text: str) -> list[str]:
    flags = shlex.split(flag_text, posix=True)
    normalized: list[str] = []
    for flag in flags:
        forward = flag.replace("\\", "/")
        if forward.startswith("-I") and forward.rstrip("/").endswith("/firmware"):
            normalized.append("-I<repo>/firmware")
        else:
            normalized.append(flag)
    return normalized


def parse_cli_version(output: str) -> str:
    version = re.search(r"\bVersion:\s*(\S+)", output)
    commit = re.search(r"\bCommit:\s*(\S+)", output)
    if not version or not commit:
        fail("could not parse arduino-cli version and commit")
    return f"{version.group(1)}+{commit.group(1)}"


def parse_core_version(output: str) -> str:
    for line in output.splitlines():
        fields = line.split()
        if fields and fields[0] == "esp32:esp32" and len(fields) >= 2:
            return fields[1]
    fail("esp32:esp32 platform is not installed")
    raise AssertionError("unreachable")


def parse_library_versions(output: str) -> tuple[str, dict[str, str]]:
    versions: dict[str, str] = {}
    powerfeather = ""
    for line in output.splitlines():
        for name in (*LIBRARY_NAMES, "PowerFeather-SDK"):
            if not line.startswith(name):
                continue
            remainder = line[len(name) :].strip().split()
            if not remainder:
                continue
            if name == "PowerFeather-SDK":
                powerfeather = remainder[0]
            else:
                versions[name] = remainder[0]
    missing = [name for name in LIBRARY_NAMES if name not in versions]
    if missing or not powerfeather:
        fail(
            "missing firmware dependency versions: "
            + ", ".join([*missing, *([] if powerfeather else ["PowerFeather-SDK"])])
        )
    return powerfeather, {name: versions[name] for name in LIBRARY_NAMES}


def clean_commit(repo_root: Path) -> tuple[str, str]:
    dirty = run(
        ["git", "status", "--porcelain=v1", "--untracked-files=all"], cwd=repo_root
    )
    if dirty.strip():
        fail("source tree is dirty; commit or remove every change before artifact build")
    commit = run(["git", "rev-parse", "HEAD"], cwd=repo_root).strip()
    committed_at = run(["git", "show", "-s", "--format=%cI", "HEAD"], cwd=repo_root).strip()
    try:
        utc_date = (
            datetime.fromisoformat(committed_at.replace("Z", "+00:00"))
            .astimezone(timezone.utc)
            .strftime("%y%m%d")
        )
    except ValueError:
        fail(f"could not parse commit timestamp: {committed_at!r}")
    return commit, utc_date


def build_recipe(
    *,
    commit: str,
    variant: str,
    flags: list[str],
    arduino_cli: str,
    esp32_platform: str,
    powerfeather_sdk: str,
    libraries: dict[str, str],
) -> dict[str, Any]:
    return {
        "schema": SCHEMA,
        "git_commit": commit,
        "variant": variant,
        "fqbn": FQBN,
        "compile_flags": flags,
        "arduino_cli": arduino_cli,
        "esp32_platform": esp32_platform,
        "powerfeather_sdk": powerfeather_sdk,
        "libraries": libraries,
    }


def prepare(args: argparse.Namespace) -> int:
    repo_root = args.repo_root.resolve()
    commit, utc_date = clean_commit(repo_root)
    cli_version = parse_cli_version(run(["arduino-cli", "version"]))
    core_version = parse_core_version(run(["arduino-cli", "core", "list"]))
    powerfeather, libraries = parse_library_versions(run(["arduino-cli", "lib", "list"]))
    flags = normalize_compile_flags(args.flags)
    recipe = build_recipe(
        commit=commit,
        variant=args.variant,
        flags=flags,
        arduino_cli=cli_version,
        esp32_platform=core_version,
        powerfeather_sdk=powerfeather,
        libraries=libraries,
    )
    recipe_bytes = canonical_recipe_bytes(recipe)
    recipe_sha = sha256_bytes(recipe_bytes)
    revision = f"fx-{utc_date}-{recipe_sha[:7]}-{args.variant}"
    artifact_dir = args.artifact_root.resolve() / revision
    if artifact_dir.exists():
        fail(f"immutable artifact path already exists: {artifact_dir}")
    artifact_dir.mkdir(parents=True, exist_ok=False)
    (artifact_dir / "recipe.json").write_bytes(recipe_bytes)
    with (artifact_dir / ".artifact-in-progress").open(
        "w", encoding="ascii", newline="\n"
    ) as handle:
        handle.write(f"fw_rev={revision}\nrecipe_sha256={recipe_sha}\n")
    print(f"{revision}\t{artifact_dir.as_posix()}\t{recipe_sha}")
    return 0


def expected_flag_text(flag: str, repo_root: Path) -> str:
    if flag == "-I<repo>/firmware":
        return "-I" + (repo_root / "firmware").as_posix()
    return flag


def atomic_text(path: Path, value: str) -> None:
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", encoding="ascii", newline="\n") as handle:
        handle.write(value)
    os.replace(temporary, path)


def finalize(args: argparse.Namespace) -> int:
    repo_root = args.repo_root.resolve()
    artifact_dir = args.artifact_dir.resolve()
    recipe_path = artifact_dir / "recipe.json"
    marker = artifact_dir / ".artifact-in-progress"
    if not recipe_path.is_file() or not marker.is_file():
        fail("artifact path lacks its recipe or in-progress marker")
    recipe_bytes = recipe_path.read_bytes()
    try:
        recipe = json.loads(recipe_bytes.decode("ascii"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        fail(f"invalid recipe.json: {exc}")
    if canonical_recipe_bytes(recipe) != recipe_bytes:
        fail("recipe.json is not canonical compact ASCII JSON plus one LF")
    recipe_sha = sha256_bytes(recipe_bytes)
    revision = artifact_dir.name
    commit, utc_date = clean_commit(repo_root)
    if commit != recipe.get("git_commit"):
        fail("source commit changed between artifact prepare and finalize")
    expected_revision = f"fx-{utc_date}-{recipe_sha[:7]}-{recipe['variant']}"
    if revision != expected_revision:
        fail(f"artifact directory {revision} != recipe-derived {expected_revision}")
    if recipe.get("schema") != SCHEMA or recipe.get("fqbn") != FQBN:
        fail("recipe schema/FQBN does not match this tool")

    identity = artifact_dir / "fixture_build_identity.h"
    options = artifact_dir / "build.options.json"
    binary = artifact_dir / "fixture.ino.bin"
    if not identity.is_file() or not options.is_file() or not binary.is_file():
        fail("compiled artifact lacks identity header, build options, or binary")
    expected_header = f'#pragma once\n#define RES_FIXTURE_VERSION "{revision}"\n'
    if identity.read_text(encoding="ascii").replace("\r\n", "\n") != expected_header:
        fail("embedded identity header does not exactly match the artifact revision")
    options_text = options.read_text(encoding="utf-8").replace("\\", "/")
    if revision not in options_text:
        fail("build options do not contain the artifact revision")
    for flag in recipe["compile_flags"]:
        expected = expected_flag_text(flag, repo_root).replace("\\", "/")
        if expected not in options_text:
            fail(f"build options do not contain recipe flag: {flag}")
    if revision.encode("ascii") not in binary.read_bytes():
        fail("compiled binary does not contain its reported revision")

    binary_sha = sha256_file(binary)
    binary_bytes = binary.stat().st_size
    commission_default = (
        "listener"
        if "-DRES_BASIC_LISTENER=1" in recipe["compile_flags"]
        else "strict"
    )
    manifest = {
        "schema": SCHEMA,
        "fw_rev": revision,
        "variant": recipe["variant"],
        "recipe_sha256": recipe_sha,
        "git_commit": recipe["git_commit"],
        "git_dirty": False,
        "fqbn": recipe["fqbn"],
        "compile_flags": recipe["compile_flags"],
        "channel_default": args.channel_default,
        "profile_default": args.profile_default,
        "commission_idle_default": commission_default,
        "wifi_profile": args.wifi_profile,
        "toolchain": {
            "arduino_cli": recipe["arduino_cli"],
            "esp32_platform": recipe["esp32_platform"],
            "powerfeather_sdk": recipe["powerfeather_sdk"],
        },
        "libraries": recipe["libraries"],
        "binary": {
            "file": binary.name,
            "bytes": binary_bytes,
            "sha256": binary_sha,
        },
        "built_utc": datetime.now(timezone.utc).isoformat(timespec="seconds").replace(
            "+00:00", "Z"
        ),
    }
    atomic_text(
        artifact_dir / "manifest.json",
        json.dumps(manifest, ensure_ascii=True, indent=2) + "\n",
    )
    atomic_text(artifact_dir / "sha256.txt", f"{binary_sha} *{binary.name}\n")

    check_manifest = json.loads((artifact_dir / "manifest.json").read_text("ascii"))
    check_hash = (artifact_dir / "sha256.txt").read_text("ascii").split()[0]
    if (
        check_manifest["recipe_sha256"] != recipe_sha
        or check_manifest["binary"]["sha256"] != binary_sha
        or check_hash != binary_sha
    ):
        fail("post-write manifest/hash cross-check failed")
    marker.unlink()
    print(
        f"ARTIFACT_READY fw_rev={revision} sha256={binary_sha} "
        f"bytes={binary_bytes} path={artifact_dir.as_posix()}"
    )
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    prep = commands.add_parser("prepare")
    prep.add_argument("--repo-root", type=Path, required=True)
    prep.add_argument("--artifact-root", type=Path, required=True)
    prep.add_argument("--variant", choices=("p", "b", "t"), required=True)
    prep.add_argument("--flags", required=True)

    done = commands.add_parser("finalize")
    done.add_argument("--repo-root", type=Path, required=True)
    done.add_argument("--artifact-dir", type=Path, required=True)
    done.add_argument("--channel-default", type=int, required=True)
    done.add_argument("--profile-default", choices=("commission", "field"), required=True)
    done.add_argument("--wifi-profile", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.command == "prepare":
        return prepare(args)
    return finalize(args)


if __name__ == "__main__":
    raise SystemExit(main())
