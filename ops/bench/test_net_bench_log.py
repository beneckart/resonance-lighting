#!/usr/bin/env python3
"""Focused output-safety tests for net_bench_log.py."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("net_bench_log.py")


class LoggerOutputSafetyTests(unittest.TestCase):
    def run_logger(self, path, *extra):
        return subprocess.run(
            [sys.executable, str(SCRIPT), "--out", str(path), "--duration", "0",
             "--port", "0", *extra],
            capture_output=True, text=True, check=False,
        )

    @staticmethod
    def rows(path):
        return [json.loads(line) for line in Path(path).read_text(encoding="utf-8").splitlines()]

    def test_new_output_is_exclusive_create_with_start_boundary(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "new.jsonl"
            result = self.run_logger(
                path, "--run-id", "durable-run", "--site", "test",
                "--operator", "alice", "--battery", "lfp", "--tx-rate", "1",
                "--notes", "original run",
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            rows = self.rows(path)
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["src"], "segment")
            self.assertEqual(rows[0]["segment_event"], "start")
            self.assertEqual(rows[0]["segment_index"], 1)
            self.assertEqual(rows[0]["run_id"], "durable-run")

    def test_existing_output_is_refused_and_unchanged_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "existing.jsonl"
            original = '{"sentinel": true}\n'
            path.write_text(original, encoding="utf-8")
            result = self.run_logger(path)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("output already exists", result.stderr)
            self.assertEqual(path.read_text(encoding="utf-8"), original)

    def test_append_preserves_identity_and_adds_resume_boundary(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "resume.jsonl"
            created = self.run_logger(
                path, "--run-id", "durable-run", "--site", "test",
                "--operator", "alice", "--battery", "lfp", "--tx-rate", "1",
                "--notes", "original run",
            )
            self.assertEqual(created.returncode, 0, created.stderr)
            resumed = self.run_logger(
                path, "--append", "--segment-notes", "host reboot",
            )
            self.assertEqual(resumed.returncode, 0, resumed.stderr)
            rows = self.rows(path)
            self.assertEqual([row["segment_event"] for row in rows], ["start", "resume"])
            self.assertEqual(rows[1]["segment_index"], 2)
            self.assertEqual(rows[1]["run_id"], "durable-run")
            self.assertEqual(rows[1]["site"], "test")
            self.assertEqual(rows[1]["operator"], "alice")
            self.assertEqual(rows[1]["segment_notes"], "host reboot")
            self.assertEqual(rows[1]["previous_ts_utc"], rows[0]["ts_utc"])

    def test_append_rejects_a_malformed_tail_without_changing_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "broken.jsonl"
            original = (
                '{"run_id":"r","site":"s","operator":"o","battery":"b",'
                '"topology":"t","tx_rate_hz":1,"notes":"n"}\nnot-json'
            )
            path.write_text(original, encoding="utf-8")
            result = self.run_logger(path, "--append")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("last non-empty line is not valid JSON", result.stderr)
            self.assertEqual(path.read_text(encoding="utf-8"), original)

    def test_overwrite_requires_explicit_flag(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "overwrite.jsonl"
            path.write_text('{"sentinel": true}\n', encoding="utf-8")
            result = self.run_logger(path, "--overwrite", "--run-id", "replacement")
            self.assertEqual(result.returncode, 0, result.stderr)
            rows = self.rows(path)
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["segment_event"], "overwrite")
            self.assertEqual(rows[0]["run_id"], "replacement")


if __name__ == "__main__":
    unittest.main()
