#!/usr/bin/env python3
"""Host-only tests for fleet OTA target policy."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import fleet_dashboard_ota as ota


REGISTRY_HEADER = "fixture_id,mac,role\n"


class SpecialTargetPolicyTests(unittest.TestCase):
    def registry(self, body: str) -> tuple[tempfile.TemporaryDirectory, Path]:
        temp = tempfile.TemporaryDirectory()
        path = Path(temp.name) / "registry.csv"
        path.write_text(REGISTRY_HEADER + body, encoding="utf-8")
        return temp, path

    def test_normal_target_needs_no_acknowledgement(self):
        temp, registry = self.registry("ABC123,00:00:00:AB:C1:23,downlight\n")
        self.addCleanup(temp.cleanup)
        ota.validate_special_targets(["ABC123"], [], registry)

    def test_magic_wand_requires_exact_acknowledgement(self):
        temp, registry = self.registry("F40344,68:EE:8F:F4:03:44,magic_wand\n")
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "--allow-special-target F40344"):
            ota.validate_special_targets(["F40344"], [], registry)
        ota.validate_special_targets(["F40344"], ["f40344"], registry)

    def test_magic_wand_must_be_only_target(self):
        temp, registry = self.registry(
            "F40344,68:EE:8F:F4:03:44,magic_wand\n"
            "ABC123,00:00:00:AB:C1:23,downlight\n"
        )
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "must be the only OTA target"):
            ota.validate_special_targets(
                ["F40344", "ABC123"], ["F40344"], registry
            )

    def test_acknowledgement_must_name_a_target(self):
        temp, registry = self.registry("ABC123,00:00:00:AB:C1:23,downlight\n")
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "names a non-target"):
            ota.validate_special_targets(["ABC123"], ["F40344"], registry)


if __name__ == "__main__":
    unittest.main()
