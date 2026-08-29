#!/usr/bin/env python3
"""Golden, compile-free contract for fixture artifact recipe bytes."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from argparse import Namespace
from pathlib import Path


FIXTURE_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(FIXTURE_DIR))

import artifact_recipe as artifact  # noqa: E402


recipe = {
    "schema": 1,
    "git_commit": "c544bf6bdfee15e7b1fcf25db350e8bee6b1bde7",
    "variant": "b",
    "fqbn": "esp32:esp32:esp32s3_powerfeather",
    "compile_flags": [
        "-DPOWERFEATHER_BOARD_V2=1",
        "-I<repo>/firmware",
        "-DRES_PF_PRECHARGE_MA=300",
        "-DRES_CHANNEL=11",
        "-DRES_PROFILE_DEFAULT=PROFILE_PROD",
        "-DRES_BASIC_LISTENER=1",
    ],
    "arduino_cli": "1.5.1+01f3d4f2b",
    "esp32_platform": "3.3.7",
    "powerfeather_sdk": "2.1.0",
    "libraries": {
        "Adafruit BusIO": "1.17.4",
        "Adafruit MSA301": "1.1.4",
        "Adafruit NeoPixel": "1.15.5",
        "Adafruit Unified Sensor": "1.1.15",
        "SparkFun Qwiic TMF882X Library": "1.0.2",
    },
}

canonical = artifact.canonical_recipe_bytes(recipe)
expected = "d37403418522644bbbe7163cb21108fd3612f88ad1021d64c1e4615199cb41f8"
assert canonical.endswith(b"\n")
assert not canonical.endswith(b"\n\n")
assert hashlib.sha256(canonical).hexdigest() == expected
assert artifact.sha256_bytes(canonical) == expected
assert artifact.sha256_bytes(canonical[:-1]) != expected

flags = artifact.normalize_compile_flags(
    "-DPOWERFEATHER_BOARD_V2=1 "
    "-IC:/Users/example/resonance-tree/firmware "
    "-DRES_CHANNEL=11"
)
assert flags == [
    "-DPOWERFEATHER_BOARD_V2=1",
    "-I<repo>/firmware",
    "-DRES_CHANNEL=11",
]

assert artifact.parse_cli_version(
    "arduino-cli  Version: 1.5.1 Commit: 01f3d4f2b Date: ignored"
) == "1.5.1+01f3d4f2b"
assert artifact.parse_core_version(
    "ID Installed Latest Name\nesp32:esp32 3.3.7 3.3.10 esp32\n"
) == "3.3.7"


def git(repo: Path, *args: str, env: dict[str, str] | None = None) -> str:
    completed = subprocess.run(
        ["git", *args],
        cwd=repo,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    assert completed.returncode == 0, completed.stdout
    return completed.stdout.strip()


with tempfile.TemporaryDirectory(prefix="artifact-contract-") as raw_temp:
    repo = Path(raw_temp)
    (repo / ".gitignore").write_text("build/\n", encoding="ascii")
    (repo / "seed.txt").write_text("fixture artifact contract\n", encoding="ascii")
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "Artifact Contract")
    git(repo, "config", "user.email", "artifact-contract@example.invalid")
    git(repo, "add", ".gitignore", "seed.txt")
    git(repo, "commit", "-q", "-m", "fixture")
    commit, utc_date = artifact.clean_commit(repo)
    fake_recipe = artifact.build_recipe(
        commit=commit,
        variant="b",
        flags=["-DPOWERFEATHER_BOARD_V2=1", "-I<repo>/firmware"],
        arduino_cli="1.5.1+01f3d4f2b",
        esp32_platform="3.3.7",
        powerfeather_sdk="2.1.0",
        libraries={name: "1.0.0" for name in artifact.LIBRARY_NAMES},
    )
    fake_bytes = artifact.canonical_recipe_bytes(fake_recipe)
    fake_sha = artifact.sha256_bytes(fake_bytes)
    revision = f"fx-{utc_date}-{fake_sha[:7]}-b"
    artifact_dir = repo / "build" / revision
    artifact_dir.mkdir(parents=True)
    (artifact_dir / "recipe.json").write_bytes(fake_bytes)
    (artifact_dir / ".artifact-in-progress").write_text("test\n", encoding="ascii")
    (artifact_dir / "fixture_build_identity.h").write_text(
        f'#pragma once\n#define RES_FIXTURE_VERSION "{revision}"\n', encoding="ascii"
    )
    actual_include = "-I" + (repo / "firmware").as_posix()
    (artifact_dir / "build.options.json").write_text(
        json.dumps(
            {
                "flags": f"-DPOWERFEATHER_BOARD_V2=1 {actual_include}",
                "identity": revision,
            }
        ),
        encoding="ascii",
    )
    (artifact_dir / "fixture.ino.bin").write_bytes(
        b"test-binary-with-identity:" + revision.encode("ascii")
    )
    artifact.finalize(
        Namespace(
            repo_root=repo,
            artifact_dir=artifact_dir,
            channel_default=11,
            profile_default="field",
            wifi_profile="test-v1",
        )
    )
    manifest = json.loads((artifact_dir / "manifest.json").read_text("ascii"))
    assert manifest["fw_rev"] == revision
    assert manifest["recipe_sha256"] == fake_sha
    assert manifest["binary"]["sha256"] == artifact.sha256_file(
        artifact_dir / "fixture.ino.bin"
    )
    assert not (artifact_dir / ".artifact-in-progress").exists()

print("ARTIFACT RECIPE CONTRACT PASSED")
