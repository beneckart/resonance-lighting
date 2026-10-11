#!/usr/bin/env python3
"""Check cross-platform roster generation and meaningful source-change detection."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


GENERATOR = Path(__file__).resolve().parents[1] / "tools" / "generate_health_registry.py"
REGISTRY = (
    "board,status,fixture_id,mac,battery_capacity_mah,role\n"
    "PowerFeather V2,commissioned,ABCDEF,00:11:22:AB:CD:EF,6000,canopy\n"
)
CALLSIGNS = (
    "callsign,category,assignment,fixture_id\n"
    "Kiki,pop,assigned,ABCDEF\n"
)


class RegistryGeneratorTest(unittest.TestCase):
    def generate(self, registry, callsigns):
        with tempfile.TemporaryDirectory() as directory:
            registry_path = Path(directory) / "registry.csv"
            callsigns_path = Path(directory) / "callsigns.csv"
            registry_path.write_bytes(registry.encode("utf-8"))
            callsigns_path.write_bytes(callsigns.encode("utf-8"))
            return subprocess.check_output(
                [sys.executable, str(GENERATOR), str(registry_path), str(callsigns_path)]
            )

    def test_checkout_line_endings_do_not_change_header(self):
        expected = self.generate(REGISTRY, CALLSIGNS)
        self.assertNotIn(b"\r", expected)
        for registry in (REGISTRY, REGISTRY.replace("\n", "\r\n")):
            for callsigns in (CALLSIGNS, CALLSIGNS.replace("\n", "\r\n")):
                with self.subTest(registry_crlf="\r" in registry,
                                  callsigns_crlf="\r" in callsigns):
                    self.assertEqual(expected, self.generate(registry, callsigns))

    def test_real_source_changes_still_change_header(self):
        original = self.generate(REGISTRY, CALLSIGNS)
        changed_registry = self.generate(REGISTRY.replace("6000", "15000"), CALLSIGNS)
        changed_callsigns = self.generate(REGISTRY, CALLSIGNS.replace("Kiki", "Kairi"))
        for changed, digest_name in (
            (changed_registry, b"kHealthRegistryCsvSha256"),
            (changed_callsigns, b"kCallsignsCsvSha256"),
        ):
            original_digest = next(line for line in original.splitlines() if digest_name in line)
            changed_digest = next(line for line in changed.splitlines() if digest_name in line)
            self.assertNotEqual(original_digest, changed_digest)
        self.assertIn(b"15000", changed_registry)
        self.assertIn(b'"Kairi"', changed_callsigns)


if __name__ == "__main__":
    unittest.main()
